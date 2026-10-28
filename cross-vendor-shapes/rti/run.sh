#!/usr/bin/env bash
# Cross-vendor proof: ZeroDDS <-> RTI Connext DDS on the ShapesDemo `Square`
# topic, both directions, on one host. Prints the received-sample counts.
#
# Prereqs:
#   * the ZeroDDS side built:   (cd ../zerodds && cargo build --release)
#   * RTI Connext DDS with a runtime licence: NDDSHOME set, and
#     $NDDSHOME/rti_license.dat readable (rtiddsgen needs none, the runtime
#     does). The RTI peer is generated and built here from ShapeType.idl the
#     first time; see README.md for what the generation does.
#
# Env:
#   NDDSHOME    Connext install    (default /opt/rti.com/rti_connext_dds-7.7.0)
#   RTI_ARCH    Connext target     (default x64Linux4gcc8.5.0)
#   DOMAIN      DDS domain id      (default 91)
#   PCAP        write a capture of the run here; needs tcpdump and root
#
# Domain isolation: a DDS domain is a UDP port band (RTPS 2.5 §9.6.1.1), so a
# stray participant on the domain is counted as this run's own result. The band
# is checked before anything starts and the run aborts rather than measuring
# into someone else's traffic.
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
NDDSHOME="${NDDSHOME:-/opt/rti.com/rti_connext_dds-7.7.0}"
RTI_ARCH="${RTI_ARCH:-x64Linux4gcc8.5.0}"
DOMAIN="${DOMAIN:-91}"
OBJS="$HERE/objs/$RTI_ARCH"

# The ZeroDDS side is ../zerodds, generated from the same ShapeType.idl as
# the peer: the four-member @final ShapesDemo type.
ZD="${CARGO_TARGET_DIR:-$HERE/../zerodds/target}/release"
ZS="$ZD/shapes_sub"
ZP="$ZD/shapes_pub"
for b in "$ZS" "$ZP"; do
    [ -x "$b" ] || { echo "missing $b — (cd ../zerodds && cargo build --release)"; exit 1; }
done

# ---- the band this run will use has to be free -----------------------------
BASE=$((7400 + 250 * DOMAIN)); HI=$((BASE + 249))
if command -v ss >/dev/null 2>&1; then
    BUSY="$(ss -uln 2>/dev/null | awk -v b="$BASE" -v h="$HI" \
        '{n=$5; sub(/.*:/,"",n); if (n+0>=b && n+0<=h) print n}' | sort -un)"
    if [ -n "$BUSY" ]; then
        echo "ABORT: the RTPS band of domain $DOMAIN ($BASE-$HI) is in use:"
        echo "$BUSY"
        ss -ulnp 2>/dev/null | awk -v b="$BASE" -v h="$HI" \
            '{n=$5; sub(/.*:/,"",n); if (n+0>=b && n+0<=h) print}'
        exit 1
    fi
    echo "domain $DOMAIN free, band $BASE-$HI empty"
else
    echo "ABORT: ss(8) is not installed, so this host cannot say whether the" \
         "band of domain $DOMAIN is free; it will not measure into an unknown host"
    exit 1
fi

# ---- the RTI peer ----------------------------------------------------------
[ -x "$NDDSHOME/bin/rtiddsgen" ] || {
    echo "missing $NDDSHOME/bin/rtiddsgen — set NDDSHOME to a Connext install"; exit 1; }
[ -f "$NDDSHOME/rti_license.dat" ] || {
    echo "missing $NDDSHOME/rti_license.dat — the Connext runtime needs a licence"; exit 1; }
export NDDSHOME RTI_LICENSE_FILE="${RTI_LICENSE_FILE:-$NDDSHOME/rti_license.dat}"
export LD_LIBRARY_PATH="$NDDSHOME/lib/$RTI_ARCH:${LD_LIBRARY_PATH:-}"

# -unboundedSupport: without it rtiddsgen bounds the unbounded `string color`
# at 255 (`initialize_string_typecode((255L))` in ShapeType.cxx). An RTI
# reader whose key is bounded shorter than the writer's is not assignable
# from it (XTypes 1.3 §7.2.4.4.8, Table 19), and it received nothing from
# the ZeroDDS writer, whose key is unbounded as the IDL says. The arguments
# are kept next to the build, so a peer generated with other ones is
# generated again.
GEN_ARGS="-language C++11 -example $RTI_ARCH -unboundedSupport -replace"
if [ ! -x "$OBJS/ShapeType_publisher" ] || [ ! -x "$OBJS/ShapeType_subscriber" ] ||
   [ "$(cat "$OBJS/rtiddsgen.args" 2>/dev/null)" != "$GEN_ARGS" ]; then
    echo "=== generating and building the RTI peer from ShapeType.idl ==="
    ( cd "$HERE" || exit 1
      # shellcheck disable=SC2086
      "$NDDSHOME/bin/rtiddsgen" $GEN_ARGS \
          ShapeType.idl >/tmp/xv_rti_gen.log 2>&1 || {
              echo "rtiddsgen failed:"; tail -20 /tmp/xv_rti_gen.log; exit 1; }
      # Both endpoints go on the standard ShapesDemo topic, not rtiddsgen's
      # "Example ShapeType".
      sed -i 's/"Example ShapeType"/"Square"/g' \
          ShapeType_publisher.cxx ShapeType_subscriber.cxx
      # rtiddsgen overwrites USER_QOS_PROFILES.xml; put ours back. The RTI
      # reader must opt into XCDR2 (ZeroDDS's writer sends it) and the
      # participant must stay on UDPv4 — same-host RTI otherwise delivers over
      # shared memory, which no foreign peer can read.
      cp "$HERE/USER_QOS_PROFILES.rti.xml" "$HERE/USER_QOS_PROFILES.xml"
      make -f "makefile_ShapeType_$RTI_ARCH" >/tmp/xv_rti_make.log 2>&1 || {
          echo "make failed:"; tail -20 /tmp/xv_rti_make.log; exit 1; }
      echo "$GEN_ARGS" >"$OBJS/rtiddsgen.args"
    ) || exit 1
fi
cp -f "$HERE/USER_QOS_PROFILES.rti.xml" "$HERE/USER_QOS_PROFILES.xml"

# ---- optional capture, on `any` ---------------------------------------------
# Not on `lo`: SPDP/SEDP leave from ephemeral metatraffic ports outside the
# domain's band, and a capture narrowed to one interface or one port range has
# reported "no SEDP" for traffic that was on the wire.
CAP=""
if [ -n "${PCAP:-}" ] && command -v tcpdump >/dev/null 2>&1; then
    tcpdump -i any -s 0 -w "$PCAP" udp >/tmp/xv_rti_tcpdump.log 2>&1 &
    CAP=$!
    sleep 1
fi

# The processes this script started, by PID. `pkill -f <name>` matched every
# command line holding the name, the shell that started this script
# included (it names the ShapesDemo binaries it builds first).
PIDS=""
# TERM first: `timeout` hands it on to its child, a KILL would orphan the child.
cleanup() {
    for p in $PIDS; do kill "$p" 2>/dev/null; done
    sleep 1
    for p in $PIDS; do kill -9 "$p" 2>/dev/null; wait "$p" 2>/dev/null; done
    PIDS=""
}
finish() { cleanup; if [ -n "$CAP" ]; then kill "$CAP" 2>/dev/null; wait "$CAP" 2>/dev/null; fi; }
trap finish EXIT

echo "=== A) RTI pub -> ZeroDDS sub (domain $DOMAIN) ==="
"$ZS" Square "$DOMAIN" >/tmp/xv_rti_zs.log 2>&1 &
PIDS="$PIDS $!"
sleep 2.5
timeout 12 "$OBJS/ShapeType_publisher" -d "$DOMAIN" -s 40 >/tmp/xv_rti_rp.log 2>&1
sleep 1
cleanup; sleep 1
A=$(grep -c '<-' /tmp/xv_rti_zs.log)
echo "ZeroDDS received from RTI: $A"; grep '<-' /tmp/xv_rti_zs.log | head -2
sleep 1

echo "=== B) ZeroDDS pub -> RTI sub (domain $DOMAIN) ==="
timeout 14 "$OBJS/ShapeType_subscriber" -d "$DOMAIN" -s 40 >/tmp/xv_rti_rs.log 2>&1 &
PIDS="$PIDS $!"
sleep 3
timeout 10 "$ZP" Square BLUE "$DOMAIN" >/tmp/xv_rti_zp.log 2>&1
sleep 2
cleanup; sleep 1
B=$(grep -c 'color:' /tmp/xv_rti_rs.log)
echo "RTI received from ZeroDDS: $B"; grep 'color:' /tmp/xv_rti_rs.log | head -2

echo
if [ "$A" -gt 0 ] && [ "$B" -gt 0 ]; then
    echo "RESULT: PASS (both directions; A=$A B=$B)"
else
    echo "RESULT: FAIL (A=$A B=$B)"; exit 1
fi
