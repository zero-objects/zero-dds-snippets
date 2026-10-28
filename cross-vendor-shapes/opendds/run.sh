#!/usr/bin/env bash
# Cross-vendor proof: ZeroDDS <-> OpenDDS on the ShapesDemo `Square` topic,
# both directions, on one host. Prints the received-sample counts.
#
# Prereqs:
#   * the ZeroDDS side built:   (cd ../zerodds && cargo build --release)
#   * an OpenDDS source tree with RTPS, built, under DDS_ROOT (its setenv.sh
#     puts opendds_idl, ACE/TAO and the libraries on the paths), and cmake.
#     The peer (publisher.cpp, subscriber.cpp) is built here the first time.
#
# Env:
#   DOMAIN      DDS domain id      (default 72)
#   DDS_ROOT    the OpenDDS tree   (required)
#
# Domain isolation: a DDS domain is a UDP port band (RTPS 2.5 §9.6.1.1), so a
# stray participant on the domain is counted as this run's own result. The band
# is checked before anything starts and the run aborts rather than measuring
# into someone else's traffic.
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
DOMAIN="${DOMAIN:-72}"
BUILD="$HERE/build"

# The ZeroDDS side is ../zerodds, generated from the same ShapeType.idl as
# the peer: the four-member @final ShapesDemo type.
ZD="${CARGO_TARGET_DIR:-$HERE/../zerodds/target}/release"
ZS="$ZD/shapes_sub"
ZP="$ZD/shapes_pub"
for b in "$ZS" "$ZP"; do
    [ -x "$b" ] || { echo "missing $b — (cd ../zerodds && cargo build --release)"; exit 1; }
done
[ -f "${DDS_ROOT:-}/setenv.sh" ] || { echo "missing \$DDS_ROOT/setenv.sh — set DDS_ROOT to a built OpenDDS tree"; exit 1; }
# setenv.sh appends to LD_LIBRARY_PATH and PATH as it finds them, which
# `set -u` refuses when the first is unset.
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
# shellcheck disable=SC1091
. "$DDS_ROOT/setenv.sh"

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

# ---- the OpenDDS peer ------------------------------------------------------
if [ ! -x "$BUILD/cmake/opendds_pub" ] || [ ! -x "$BUILD/cmake/opendds_sub" ]; then
    echo "=== generating the OpenDDS type support and building the peer ==="
    mkdir -p "$BUILD"
    cmake -S "$HERE" -B "$BUILD/cmake" -DOpenDDS_ROOT="$DDS_ROOT" \
        -DCMAKE_BUILD_TYPE=Release >"$BUILD/cmake.log" 2>&1 &&
    cmake --build "$BUILD/cmake" -j >"$BUILD/build.log" 2>&1 || {
        echo "cmake failed:"; tail -30 "$BUILD/cmake.log" "$BUILD/build.log"; exit 1; }
fi

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

echo "=== A) OpenDDS pub -> ZeroDDS sub ==="
"$ZS" Square "$DOMAIN" >"$LOG/zs.log" 2>&1 &
PIDS="$PIDS $!"
sleep 2.5
( cd "$HERE" && timeout 7 "$BUILD/cmake/opendds_pub" -DCPSConfigFile rtps.ini "$DOMAIN" ) >"$LOG/op.log" 2>&1
sleep 1; cleanup
A=$(grep -c '<-' "$LOG/zs.log")
echo "ZeroDDS received from OpenDDS: $A"; grep '<-' "$LOG/zs.log" | head -2
sleep 1

echo "=== B) ZeroDDS pub -> OpenDDS sub ==="
( cd "$HERE" && exec timeout 9 "$BUILD/cmake/opendds_sub" -DCPSConfigFile rtps.ini "$DOMAIN" ) >"$LOG/os.log" 2>&1 &
PIDS="$PIDS $!"
sleep 2.5
timeout 6 "$ZP" Square BLUE "$DOMAIN" >"$LOG/zp.log" 2>&1
sleep 1; cleanup
B=$(grep -c '<-' "$LOG/os.log")
echo "OpenDDS received from ZeroDDS: $B"; grep '<-' "$LOG/os.log" | head -2

echo
echo "logs: $LOG"
if [ "$A" -gt 0 ] && [ "$B" -gt 0 ]; then
    echo "RESULT: PASS (both directions; A=$A B=$B)"
else
    echo "RESULT: FAIL (A=$A B=$B)"; exit 1
fi
