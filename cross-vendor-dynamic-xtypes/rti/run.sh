#!/usr/bin/env bash
# Reflective decode of an RTI Connext topic: the RTI publisher writes `Track`
# (../dyn_app.idl) on `TrackTopic`, and `zerodds-spy --decode` reads it with no
# compiled Rust type, building the DynamicType from the out-of-band IDL.
#
# Prints the number of samples ZeroDDS decoded and which union branches it saw:
# the point of the example is that all three (`speed` / `angle` / `flag`) come
# out, because the branch is what a reflective decoder is most likely to get
# wrong.
#
# Prereqs:
#   * zerodds-spy built:  cargo build --release -p zerodds-spy
#   * RTI Connext with a runtime licence (NDDSHOME, rti_license.dat). The peer
#     is generated from ../dyn_app.idl and built here on the first run.
#
# Env:
#   NDDSHOME    Connext install   (default /opt/rti.com/rti_connext_dds-7.7.0)
#   RTI_ARCH    Connext target    (default x64Linux4gcc8.5.0)
#   DOMAIN      DDS domain id     (default 99)
#   SPY         path to zerodds-spy (default $ZERODDS_SRC/target/release/zerodds-spy)
#   PCAP        write a capture of the run here; needs tcpdump and root
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
NDDSHOME="${NDDSHOME:-/opt/rti.com/rti_connext_dds-7.7.0}"
RTI_ARCH="${RTI_ARCH:-x64Linux4gcc8.5.0}"
DOMAIN="${DOMAIN:-99}"
# `ZERODDS_SRC` is what ci/run-examples.sh exports; outside it, the checkout is
# two levels up from this repository.
SRC="${ZERODDS_SRC:-$HERE/../../..}"
SPY="${SPY:-$SRC/target/release/zerodds-spy}"

[ -x "$SPY" ] || { echo "missing $SPY — cargo build --release -p zerodds-spy"; exit 1; }
[ -x "$NDDSHOME/bin/rtiddsgen" ] || {
    echo "missing $NDDSHOME/bin/rtiddsgen — set NDDSHOME to a Connext install"; exit 1; }
[ -f "$NDDSHOME/rti_license.dat" ] || {
    echo "missing $NDDSHOME/rti_license.dat — the Connext runtime needs a licence"; exit 1; }
export NDDSHOME RTI_LICENSE_FILE="${RTI_LICENSE_FILE:-$NDDSHOME/rti_license.dat}"
export LD_LIBRARY_PATH="$NDDSHOME/lib/$RTI_ARCH:${LD_LIBRARY_PATH:-}"

# ---- the band this run will use has to be free -----------------------------
command -v ss >/dev/null 2>&1 || {
    echo "ABORT: ss(8) is not installed, so this host cannot say whether the" \
         "band of domain $DOMAIN is free; it will not measure into an unknown host"; exit 1; }
BASE=$((7400 + 250 * DOMAIN)); HI=$((BASE + 249))
BUSY="$(ss -uln 2>/dev/null | awk -v b="$BASE" -v h="$HI" \
    '{n=$5; sub(/.*:/,"",n); if (n+0>=b && n+0<=h) print n}' | sort -un)"
if [ -n "$BUSY" ]; then
    echo "ABORT: the RTPS band of domain $DOMAIN ($BASE-$HI) is in use:"; echo "$BUSY"
    ss -ulnp 2>/dev/null | awk -v b="$BASE" -v h="$HI" \
        '{n=$5; sub(/.*:/,"",n); if (n+0>=b && n+0<=h) print}'
    exit 1
fi
echo "domain $DOMAIN free, band $BASE-$HI empty"

# ---- the RTI peer ----------------------------------------------------------
# `@mutable union` is not portable (../dyn.idl); ../dyn_app.idl is the
# @appendable variant every vendor's IDL compiler accepts, and the one this
# leg measures. No -example: pub.cxx and USER_QOS_PROFILES.xml here are ours.
if [ ! -x "$HERE/rti_dyn_pub" ]; then
    echo "=== generating the RTI type support and building the publisher ==="
    ( cd "$HERE" || exit 1
      "$NDDSHOME/bin/rtiddsgen" -language C++11 -platform "$RTI_ARCH" -replace \
          -d . ../dyn_app.idl >/tmp/xv_dyn_gen.log 2>&1 || {
              echo "rtiddsgen failed:"; tail -20 /tmp/xv_dyn_gen.log; exit 1; }
      RTIFLAGS="-DRTI_UNIX -DRTI_LINUX -DRTI_64BIT -DRTI_STATIC"
      # shellcheck disable=SC2086
      g++ -m64 -Wall -std=c++14 $RTIFLAGS -I. \
          -I"$NDDSHOME/include" -I"$NDDSHOME/include/ndds" \
          -I"$NDDSHOME/include/ndds/hpp" \
          pub.cxx dyn_app.cxx dyn_appPlugin.cxx -o rti_dyn_pub \
          -L"$NDDSHOME/lib/$RTI_ARCH" -lnddscpp2z -lnddscz -lnddscorez \
          -ldl -lm -lpthread -lrt -no-pie -rdynamic >/tmp/xv_dyn_build.log 2>&1 || {
              echo "g++ failed:"; tail -30 /tmp/xv_dyn_build.log; exit 1; }
    ) || exit 1
fi

# ---- optional capture, on `any` --------------------------------------------
# Not on `lo`: SPDP/SEDP leave from ephemeral metatraffic ports outside the
# domain's band, and a capture narrowed to one interface or one port range has
# reported "no SEDP" for traffic that was on the wire.
CAP=""
if [ -n "${PCAP:-}" ] && command -v tcpdump >/dev/null 2>&1; then
    tcpdump -i any -s 0 -w "$PCAP" udp >/tmp/xv_dyn_tcpdump.log 2>&1 &
    CAP=$!
    sleep 1
fi
finish() { [ -n "$CAP" ] && { kill "$CAP" 2>/dev/null; wait "$CAP" 2>/dev/null; }; return 0; }
trap finish EXIT

echo "=== RTI pub -> zerodds-spy reflective decode (domain $DOMAIN) ==="
"$SPY" -d "$DOMAIN" -t TrackTopic --type-file "$HERE/../dyn_app.idl" --decode \
    --duration 25s >/tmp/xv_dyn_spy.log 2>&1 &
SPYPID=$!
sleep 3
timeout 20 "$HERE/rti_dyn_pub" "$DOMAIN" 45 >/tmp/xv_dyn_rp.log 2>&1
wait "$SPYPID" 2>/dev/null

N=$(grep -c '"command"' /tmp/xv_dyn_spy.log)
SPEED=$(grep -c '"speed"' /tmp/xv_dyn_spy.log)
ANGLE=$(grep -c '"angle"' /tmp/xv_dyn_spy.log)
FLAG=$(grep -c '"flag"' /tmp/xv_dyn_spy.log)
echo "ZeroDDS reflectively decoded from RTI: $N  (speed=$SPEED angle=$ANGLE flag=$FLAG)"
grep '"command"' /tmp/xv_dyn_spy.log | head -3

echo
if [ "$N" -gt 0 ] && [ "$SPEED" -gt 0 ] && [ "$ANGLE" -gt 0 ] && [ "$FLAG" -gt 0 ]; then
    echo "RESULT: PASS ($N samples, all three union branches)"
else
    echo "RESULT: FAIL (n=$N speed=$SPEED angle=$ANGLE flag=$FLAG)"
    tail -20 /tmp/xv_dyn_spy.log | sed 's/^/  spy| /'
    tail -10 /tmp/xv_dyn_rp.log | sed 's/^/  rti| /'
    exit 1
fi
