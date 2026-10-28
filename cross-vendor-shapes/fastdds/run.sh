#!/usr/bin/env bash
# Cross-vendor proof: ZeroDDS <-> eProsima Fast DDS on the ShapesDemo `Square`
# topic, both directions, on one host. Prints the received-sample counts.
#
# Prereqs:
#   * the ZeroDDS side built:   (cd ../zerodds && cargo build --release)
#   * Fast DDS 3 installed under FASTDDS_HOME, fastddsgen and cmake. The peer
#     (pub.cxx, sub.cxx) is generated from ShapeType.idl and built here the
#     first time.
#
# Env:
#   DOMAIN      DDS domain id      (default 71)
#   FASTDDS_HOME  Fast DDS install (default /opt/fastdds)
#   FASTDDSGEN    fastddsgen       (default: the one on PATH)
#
# Domain isolation: a DDS domain is a UDP port band (RTPS 2.5 §9.6.1.1), so a
# stray participant on the domain is counted as this run's own result. The band
# is checked before anything starts and the run aborts rather than measuring
# into someone else's traffic.
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
# The Fast DDS reader announces no TypeConsistencyEnforcement (PID 0x0074).
# DDS-XTypes 1.3 §7.6.3.4.1 has a writer assume DISALLOW_TYPE_COERCION for it,
# which is the ZeroDDS default (xtypes.type_consistency_absent=disallow).
# The ShapeTypes of the two sides are assignable but not equivalent.
# Under DISALLOW the ZeroDDS writer refuses the reader ("type.inconsistent")
# and the ZeroDDS -> Fast DDS direction receives nothing; this leg reads the
# absent policy as ALLOW_TYPE_COERCION.
export ZERODDS_TYPE_CONSISTENCY_ABSENT="${ZERODDS_TYPE_CONSISTENCY_ABSENT:-allow}"
DOMAIN="${DOMAIN:-71}"
FASTDDS_HOME="${FASTDDS_HOME:-/opt/fastdds}"
FASTDDSGEN="${FASTDDSGEN:-fastddsgen}"
BUILD="$HERE/build"

# The ZeroDDS side is ../zerodds, generated from the same ShapeType.idl as
# the peer: the four-member @final ShapesDemo type.
ZD="${CARGO_TARGET_DIR:-$HERE/../zerodds/target}/release"
ZS="$ZD/shapes_sub"
ZP="$ZD/shapes_pub"
for b in "$ZS" "$ZP"; do
    [ -x "$b" ] || { echo "missing $b — (cd ../zerodds && cargo build --release)"; exit 1; }
done
command -v "$FASTDDSGEN" >/dev/null 2>&1 || { echo "missing fastddsgen — set FASTDDSGEN"; exit 1; }
[ -d "$FASTDDS_HOME/lib" ] || { echo "missing $FASTDDS_HOME/lib — set FASTDDS_HOME to a Fast DDS install"; exit 1; }
export LD_LIBRARY_PATH="$FASTDDS_HOME/lib:${LD_LIBRARY_PATH:-}"

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

# ---- the Fast DDS peer -----------------------------------------------------
if [ ! -x "$BUILD/bin/shapes_fd_pub" ] || [ ! -x "$BUILD/bin/shapes_fd_sub" ]; then
    echo "=== generating the Fast DDS type support and building the peer ==="
    mkdir -p "$BUILD/src"
    cp "$HERE/ShapeType.idl" "$HERE/pub.cxx" "$HERE/sub.cxx" "$HERE/CMakeLists.txt" "$BUILD/src/"
    ( cd "$BUILD/src" || exit 1
      "$FASTDDSGEN" -replace ShapeType.idl >gen.log 2>&1 || {
          echo "fastddsgen failed:"; tail -20 gen.log; exit 1; }
      cmake -S . -B ../cmake -DCMAKE_PREFIX_PATH="$FASTDDS_HOME" \
          -DCMAKE_BUILD_TYPE=Release >cmake.log 2>&1 &&
      cmake --build ../cmake -j >build.log 2>&1 || {
          echo "cmake failed:"; tail -30 cmake.log build.log; exit 1; }
      mkdir -p ../bin && cp ../cmake/shapes_fd_pub ../cmake/shapes_fd_sub ../bin/
    ) || exit 1
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

echo "=== A) Fast DDS pub -> ZeroDDS sub ==="
"$ZS" Square "$DOMAIN" >"$LOG/zs.log" 2>&1 &
PIDS="$PIDS $!"
sleep 2.5
timeout 7 "$BUILD/bin/shapes_fd_pub" Square GREEN "$DOMAIN" >"$LOG/fp.log" 2>&1
sleep 1; cleanup
A=$(grep -c '<-' "$LOG/zs.log")
echo "ZeroDDS received from Fast DDS: $A"; grep '<-' "$LOG/zs.log" | head -2
sleep 1

echo "=== B) ZeroDDS pub -> Fast DDS sub ==="
timeout 9 "$BUILD/bin/shapes_fd_sub" Square "$DOMAIN" >"$LOG/fs.log" 2>&1 &
PIDS="$PIDS $!"
sleep 2.5
timeout 6 "$ZP" Square BLUE "$DOMAIN" >"$LOG/zp.log" 2>&1
sleep 1; cleanup
B=$(grep -c '<-' "$LOG/fs.log")
echo "Fast DDS received from ZeroDDS: $B"; grep '<-' "$LOG/fs.log" | head -2

echo
echo "logs: $LOG"
if [ "$A" -gt 0 ] && [ "$B" -gt 0 ]; then
    echo "RESULT: PASS (both directions; A=$A B=$B)"
else
    echo "RESULT: FAIL (A=$A B=$B)"; exit 1
fi
