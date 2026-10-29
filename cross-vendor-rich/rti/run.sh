#!/usr/bin/env bash
# Cross-vendor proof: ZeroDDS <-> RTI Connext DDS on the rich type, in all
# three XTypes extensibilities, both directions. Prints the received-sample
# count of every cell.
#
# Six cells: {@final, @appendable, @mutable} x {RTI -> ZeroDDS, ZeroDDS -> RTI}.
# Each cell gets a domain of its own, because a DDS domain is a UDP port band
# (RTPS 2.5 §9.6.1.1) and two cells on one domain read each other's traffic.
#
# Prereqs:
#   * cargo: the ZeroDDS side is built here, in RTI's CDR dialect
#     (ZERODDS_RICH_DIALECT=rti, see ../zerodds/build.rs), into a target
#     directory of its own
#   * RTI Connext with a runtime licence (NDDSHOME, rti_license.dat) and the
#     peer built:               make -f makefile_Rich_$RTI_ARCH
#     run.sh generates and builds it when it is not there; see README.md.
#
# Env:
#   NDDSHOME      Connext install   (default /opt/rti.com/rti_connext_dds-7.7.0)
#   RTI_ARCH      Connext target    (default x64Linux4gcc8.5.0)
#   DOMAIN_BASE   first domain id   (default 92; six consecutive ids are used)
#   BLOB          sequence<octet> length (default 50)
#   PCAP          write a capture of the run here; needs tcpdump and root
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
NDDSHOME="${NDDSHOME:-/opt/rti.com/rti_connext_dds-7.7.0}"
RTI_ARCH="${RTI_ARCH:-x64Linux4gcc8.5.0}"
DOMAIN_BASE="${DOMAIN_BASE:-92}"
BLOB="${BLOB:-50}"
OBJS="$HERE/objs/$RTI_ARCH"
ZD_TARGET="$HERE/build/zerodds-target"
ZD="$ZD_TARGET/release"

# ---- the ZeroDDS side, in RTI's dialect ------------------------------------
# RTI writes a @mutable member it cannot size in advance under PID_EXTENDED
# and declares its length rounded up to four (the unbounded `blob`: 54 octets
# of value, length 56). A codec built in the specification's dialect reads
# the length as the member's own (XTypes 1.3 §7.4.1.2) and refuses the
# sample as a framing error, so RTI -> ZeroDDS @mutable delivers nothing.
# The dialect is a build setting, so this leg has binaries of its own.
( cd "$HERE/../zerodds" &&
  CARGO_TARGET_DIR="$ZD_TARGET" ZERODDS_RICH_DIALECT=rti cargo build --quiet --release ) || {
    echo "cargo build of the ZeroDDS side (ZERODDS_RICH_DIALECT=rti) failed"; exit 1; }
for b in "$ZD/rich_pub" "$ZD/rich_sub"; do
    [ -x "$b" ] || { echo "missing $b after the build"; exit 1; }
done
[ -x "$NDDSHOME/bin/rtiddsgen" ] || {
    echo "missing $NDDSHOME/bin/rtiddsgen — set NDDSHOME to a Connext install"; exit 1; }
[ -f "$NDDSHOME/rti_license.dat" ] || {
    echo "missing $NDDSHOME/rti_license.dat — the Connext runtime needs a licence"; exit 1; }
export NDDSHOME RTI_LICENSE_FILE="${RTI_LICENSE_FILE:-$NDDSHOME/rti_license.dat}"
export LD_LIBRARY_PATH="$NDDSHOME/lib/$RTI_ARCH:${LD_LIBRARY_PATH:-}"

# ---- the six domains this run will use have to be free ---------------------
command -v ss >/dev/null 2>&1 || {
    echo "ABORT: ss(8) is not installed, so this host cannot say whether the" \
         "domains are free; it will not measure into an unknown host"; exit 1; }
BUSY=""
for i in 0 1 2 3 4 5; do
    d=$((DOMAIN_BASE + i)); lo=$((7400 + 250 * d)); hi=$((lo + 249))
    hits="$(ss -uln 2>/dev/null | awk -v b="$lo" -v h="$hi" \
        '{n=$5; sub(/.*:/,"",n); if (n+0>=b && n+0<=h) print n}' | sort -un | tr '\n' ' ')"
    [ -n "$hits" ] && BUSY="$BUSY domain $d ($lo-$hi): $hits;"
done
if [ -n "$BUSY" ]; then
    echo "ABORT: the RTPS band of a domain this run needs is in use:$BUSY"
    ss -ulnp 2>/dev/null | head -40
    exit 1
fi
echo "domains $DOMAIN_BASE..$((DOMAIN_BASE + 5)) free"

if [ ! -x "$OBJS/Rich_publisher" ] || [ ! -x "$OBJS/Rich_subscriber" ]; then
    echo "=== generating the RTI type support and building the peer ==="
    ( cd "$HERE" || exit 1
      # -unboundedSupport: RTI otherwise bounds `sequence<octet>` at 100.
      # No -example: Rich_publisher.cxx / Rich_subscriber.cxx and
      # USER_QOS_PROFILES.xml in this directory are ours and must survive.
      "$NDDSHOME/bin/rtiddsgen" -language C++11 -platform "$RTI_ARCH" \
          -unboundedSupport -replace -d . Rich.idl >/tmp/xv_rich_gen.log 2>&1 || {
              echo "rtiddsgen failed:"; tail -20 /tmp/xv_rich_gen.log; exit 1; }
      make -f "makefile_Rich_$RTI_ARCH" >/tmp/xv_rich_make.log 2>&1 || {
          echo "make failed:"; tail -20 /tmp/xv_rich_make.log; exit 1; }
    ) || exit 1
fi

# ---- optional capture, on `any` --------------------------------------------
# Not on `lo`: SPDP/SEDP leave from ephemeral metatraffic ports outside the
# domain's band, and a capture narrowed to one interface or one port range has
# reported "no SEDP" for traffic that was on the wire.
CAP=""
if [ -n "${PCAP:-}" ] && command -v tcpdump >/dev/null 2>&1; then
    tcpdump -i any -s 0 -w "$PCAP" udp >/tmp/xv_rich_tcpdump.log 2>&1 &
    CAP=$!
    sleep 1
fi

# The processes this script started, by PID; `pkill -f <name>` matched every
# command line holding the name, the shell that started this script included.
# TERM first: `timeout` hands it on to its child, a KILL would orphan the child.
PIDS=""
cleanup() {
    for p in $PIDS; do kill "$p" 2>/dev/null; done
    sleep 1
    for p in $PIDS; do kill -9 "$p" 2>/dev/null; wait "$p" 2>/dev/null; done
    PIDS=""
}
finish() { cleanup; if [ -n "$CAP" ]; then kill "$CAP" 2>/dev/null; wait "$CAP" 2>/dev/null; fi; }
trap finish EXIT

FAILED=""
i=0
for ext in f a m; do
    dom=$((DOMAIN_BASE + i)); i=$((i + 1))
    echo "=== A/$ext) RTI pub -> ZeroDDS sub (domain $dom) ==="
    "$ZD/rich_sub" "$dom" "$ext" >"/tmp/xv_rich_${ext}_zs.log" 2>&1 &
    PIDS="$PIDS $!"
    sleep 2.5
    timeout 12 "$OBJS/Rich_publisher" "$dom" "$ext" "$BLOB" \
        >"/tmp/xv_rich_${ext}_rp.log" 2>&1
    sleep 1; cleanup; sleep 1
    n="$(grep -c 'integrity=OK' "/tmp/xv_rich_${ext}_zs.log")"
    bad="$(grep -c 'integrity=CORRUPT' "/tmp/xv_rich_${ext}_zs.log")"
    echo "ZeroDDS received from RTI (@$ext): $n  (corrupt: $bad)"
    grep 'integrity=' "/tmp/xv_rich_${ext}_zs.log" | head -1
    { [ "$n" -gt 0 ] && [ "$bad" = 0 ]; } || FAILED="$FAILED rti->zerodds/@$ext(n=$n,corrupt=$bad)"
    sleep 1
done

for ext in f a m; do
    dom=$((DOMAIN_BASE + i)); i=$((i + 1))
    echo "=== B/$ext) ZeroDDS pub -> RTI sub (domain $dom) ==="
    timeout 16 "$OBJS/Rich_subscriber" "$dom" "$ext" \
        >"/tmp/xv_rich_${ext}_rs.log" 2>&1 &
    PIDS="$PIDS $!"
    sleep 3
    timeout 10 "$ZD/rich_pub" "$dom" "$ext" "$BLOB" \
        >"/tmp/xv_rich_${ext}_zp.log" 2>&1
    sleep 2; cleanup; sleep 1
    n="$(grep -c 'integrity=OK' "/tmp/xv_rich_${ext}_rs.log")"
    bad="$(grep -c 'integrity=CORRUPT' "/tmp/xv_rich_${ext}_rs.log")"
    echo "RTI received from ZeroDDS (@$ext): $n  (corrupt: $bad)"
    grep 'integrity=' "/tmp/xv_rich_${ext}_rs.log" | head -1
    { [ "$n" -gt 0 ] && [ "$bad" = 0 ]; } || FAILED="$FAILED zerodds->rti/@$ext(n=$n,corrupt=$bad)"
    sleep 1
done

echo
if [ -z "$FAILED" ]; then
    echo "RESULT: PASS (6 cells, both directions, all three extensibilities)"
else
    echo "RESULT: FAIL —$FAILED"; exit 1
fi
