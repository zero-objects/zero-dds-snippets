#!/usr/bin/env bash
# Cross-vendor proof: ZeroDDS <-> OpenSplice Community Edition 6.9 on the
# ShapesDemo `Square` topic, both directions, on one host. Prints the
# received-sample counts.
#
# Prereqs:
#   * the ZeroDDS side built:   (cd ../zerodds && cargo build --release)
#   * OpenSplice 6.9 under OSPL_HOME (its release.com, idlpp and the classic
#     C++ API) and g++. The peer (publisher.cpp, subscriber.cpp) is built
#     here the first time.
#
# Env:
#   DOMAIN      DDS domain id              (default 73; OpenSplice takes 0-230)
#   OSPL_HOME   the OpenSplice install     (default /opt/opensplice-6.9)
#
# OpenSplice takes its domain from its configuration file, not from the
# participant call: run.sh writes one for DOMAIN and points OSPL_URI at it.
# Domain isolation: a DDS domain is a UDP port band (RTPS 2.5 §9.6.1.1), so a
# stray participant on the domain is counted as this run's own result. The
# band is checked before anything starts and the run aborts rather than
# measuring into someone else's traffic.
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
DOMAIN="${DOMAIN:-73}"
OSPL_HOME="${OSPL_HOME:-/opt/opensplice-6.9}"
BUILD="$HERE/build"

ZD="${CARGO_TARGET_DIR:-$HERE/../zerodds/target}/release"
ZS="$ZD/shapes_sub"
ZP="$ZD/shapes_pub"
for b in "$ZS" "$ZP"; do
    [ -x "$b" ] || { echo "missing $b — (cd ../zerodds && cargo build --release)"; exit 1; }
done
[ -x "$OSPL_HOME/bin/idlpp" ] || { echo "missing $OSPL_HOME/bin/idlpp — set OSPL_HOME to an OpenSplice 6.9 install"; exit 1; }
[ "$DOMAIN" -le 230 ] || { echo "OpenSplice 6.9 refuses domain ids above 230 (DOMAIN=$DOMAIN)"; exit 1; }
# release.com reads LD_LIBRARY_PATH unset, which `set -u` refuses.
set +u
# shellcheck disable=SC1091
. "$OSPL_HOME/release.com" >/dev/null
set -u

# ---- the band this run will use has to be free -----------------------------
BASE=$((7400 + 250 * DOMAIN)); HI=$((BASE + 249))
command -v ss >/dev/null 2>&1 || {
    echo "ABORT: ss(8) is not installed, so this host cannot say whether the" \
         "band of domain $DOMAIN is free; it will not measure into an unknown host"
    exit 1; }
BUSY="$(ss -uln 2>/dev/null | awk -v b="$BASE" -v h="$HI" \
    '{n=$5; sub(/.*:/,"",n); if (n+0>=b && n+0<=h) print n}' | sort -un)"
if [ -n "$BUSY" ]; then
    echo "ABORT: the RTPS band of domain $DOMAIN ($BASE-$HI) is in use:"
    echo "$BUSY"
    exit 1
fi
echo "domain $DOMAIN free, band $BASE-$HI empty"

# ---- the OpenSplice peer ---------------------------------------------------
if [ ! -x "$BUILD/opensplice_pub" ] || [ ! -x "$BUILD/opensplice_sub" ]; then
    echo "=== generating the OpenSplice type support and building the peer ==="
    mkdir -p "$BUILD"
    cp "$HERE/ShapeType.idl" "$HERE/publisher.cpp" "$HERE/subscriber.cpp" "$BUILD/"
    (
        cd "$BUILD" &&
        idlpp -S -l cpp ShapeType.idl > idlpp.log 2>&1 &&
        for side in pub sub; do
            src=publisher.cpp; [ "$side" = sub ] && src=subscriber.cpp
            g++ -std=c++11 -O1 -I. -I"$OSPL_HOME/include" -I"$OSPL_HOME/include/sys" \
                -I"$OSPL_HOME/include/dcps/C++/SACPP" -o "opensplice_$side" "$src" \
                ShapeType.cpp ShapeTypeDcps.cpp ShapeTypeDcps_impl.cpp ShapeTypeSplDcps.cpp \
                -L"$OSPL_HOME/lib" -ldcpssacpp -lddskernel -Wl,-rpath,"$OSPL_HOME/lib" || exit 1
        done
    ) > "$BUILD/build.log" 2>&1 || { echo "build failed:"; tail -30 "$BUILD/idlpp.log" "$BUILD/build.log"; exit 1; }
fi
sed "s#<Id>0</Id>#<Id>$DOMAIN</Id>#" "$OSPL_HOME/etc/config/ospl_sp_ddsi.xml" > "$BUILD/ospl.xml"
export OSPL_URI="file://$BUILD/ospl.xml"

# The processes this script started, by PID: `pkill -f <name>` matches every
# command line holding the name, the shell that started this script included.
# TERM first: `timeout` hands it on to its child, a KILL would orphan the child.
LOG="$(mktemp -d)"
PIDS=""
cleanup() {
    for p in $PIDS; do kill "$p" 2>/dev/null; done
    sleep 1
    for p in $PIDS; do kill -9 "$p" 2>/dev/null; wait "$p" 2>/dev/null; done
    PIDS=""
}
trap cleanup EXIT

# OpenSplice writes ospl-info.log and ospl-error.log into its working
# directory; both runs keep theirs in $LOG.
echo "=== A) OpenSplice pub -> ZeroDDS sub ==="
"$ZS" Square "$DOMAIN" >"$LOG/zs.log" 2>&1 &
PIDS="$PIDS $!"
sleep 2.5
( cd "$LOG" && timeout 7 "$BUILD/opensplice_pub" ) >"$LOG/op.log" 2>&1
sleep 1; cleanup
A=$(grep -c '<-' "$LOG/zs.log")
echo "ZeroDDS received from OpenSplice: $A"; grep '<-' "$LOG/zs.log" | head -2
sleep 1

echo "=== B) ZeroDDS pub -> OpenSplice sub ==="
( cd "$LOG" && exec timeout 9 "$BUILD/opensplice_sub" ) >"$LOG/os.log" 2>&1 &
PIDS="$PIDS $!"
sleep 2.5
timeout 6 "$ZP" Square BLUE "$DOMAIN" >"$LOG/zp.log" 2>&1
sleep 1; cleanup
B=$(grep -c '<-' "$LOG/os.log")
echo "OpenSplice received from ZeroDDS: $B"; grep '<-' "$LOG/os.log" | head -2

echo
echo "logs: $LOG"
if [ "$A" -gt 0 ] && [ "$B" -gt 0 ]; then
    echo "RESULT: PASS (both directions; A=$A B=$B)"
else
    echo "RESULT: FAIL (A=$A B=$B)"; exit 1
fi
