#!/usr/bin/env bash
# Cross-vendor proof: ZeroDDS <-> Cyclone DDS on the ShapesDemo `Square` topic,
# both directions, on one host. Prints the received-sample counts.
#
# Prereqs:
#   * the ZeroDDS side built:   (cd ../zerodds && cargo build --release)
#   * the Cyclone DDS Python binding: `python3 -c 'import cyclonedds'` works
#     (pip install cyclonedds, against a Cyclone DDS install).
#
# Env:
#   DOMAIN      DDS domain id      (default 70)
#   PYTHON      the interpreter with the binding (default python3)
#
# Domain isolation: a DDS domain is a UDP port band (RTPS 2.5 §9.6.1.1), so a
# stray participant on the domain is counted as this run's own result. The band
# is checked before anything starts and the run aborts rather than measuring
# into someone else's traffic.
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
DOMAIN="${DOMAIN:-70}"
PY="${PYTHON:-python3}"

# The ZeroDDS side is ../zerodds, generated from the same ShapeType.idl as
# the peer: the four-member @final ShapesDemo type.
ZD="${CARGO_TARGET_DIR:-$HERE/../zerodds/target}/release"
ZS="$ZD/shapes_sub"
ZP="$ZD/shapes_pub"
for b in "$ZS" "$ZP"; do
    [ -x "$b" ] || { echo "missing $b — (cd ../zerodds && cargo build --release)"; exit 1; }
done
"$PY" -c 'import cyclonedds' 2>/dev/null || {
    echo "missing the Cyclone DDS Python binding ($PY -c 'import cyclonedds' fails)"; exit 1; }

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

echo "=== A) Cyclone DDS pub -> ZeroDDS sub ==="
"$ZS" Square "$DOMAIN" >"$LOG/zs.log" 2>&1 &
PIDS="$PIDS $!"
sleep 2.5
timeout 7 "$PY" "$HERE/shapes_pub.py" Square GREEN "$DOMAIN" >"$LOG/cp.log" 2>&1
sleep 1; cleanup
A=$(grep -c '<-' "$LOG/zs.log")
echo "ZeroDDS received from Cyclone: $A"; grep '<-' "$LOG/zs.log" | head -2
sleep 1

echo "=== B) ZeroDDS pub -> Cyclone DDS sub ==="
timeout 9 "$PY" "$HERE/shapes_sub.py" Square "$DOMAIN" >"$LOG/cs.log" 2>&1 &
PIDS="$PIDS $!"
sleep 2.5
timeout 6 "$ZP" Square BLUE "$DOMAIN" >"$LOG/zp.log" 2>&1
sleep 1; cleanup
B=$(grep -c '<-' "$LOG/cs.log")
echo "Cyclone received from ZeroDDS: $B"; grep '<-' "$LOG/cs.log" | head -2

echo
echo "logs: $LOG"
if [ "$A" -gt 0 ] && [ "$B" -gt 0 ]; then
    echo "RESULT: PASS (both directions; A=$A B=$B)"
else
    echo "RESULT: FAIL (A=$A B=$B)"; exit 1
fi
