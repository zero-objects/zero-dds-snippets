#!/usr/bin/env bash
# Cross-vendor proof: ZeroDDS <-> CycloneDDS on the ShapesDemo `Square` topic,
# both directions, on one host. Prints the received-sample counts.
#
# Prereqs:
#   * ZeroDDS shapes examples built:
#       cargo build -p zerodds-dcps --example shapes_demo_publisher \
#                                   --example shapes_demo_subscriber
#   * CycloneDDS + its Python binding: pip install cyclonedds
#     (CYCLONEDDS_HOME / LD_LIBRARY_PATH pointing at the C library).
#
# Env:
#   ZD_TARGET   path to the cargo target dir (default: ../../../target/debug/examples)
#   DOMAIN      DDS domain id (default 0)
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
ZD="${ZD_TARGET:-$HERE/../../../target/debug/examples}"
DOMAIN="${DOMAIN:-0}"
PY="${PYTHON:-python3}"

ZS="$ZD/shapes_demo_subscriber"
ZP="$ZD/shapes_demo_publisher"
for b in "$ZS" "$ZP"; do
    [ -x "$b" ] || { echo "missing $b — build the ZeroDDS shapes examples first"; exit 1; }
done

cleanup() { pkill -9 -f shapes_demo >/dev/null 2>&1; pkill -9 -f shapes_pub.py >/dev/null 2>&1; pkill -9 -f shapes_sub.py >/dev/null 2>&1; }
trap cleanup EXIT
cleanup; sleep 1

echo "=== A) CycloneDDS pub -> ZeroDDS sub ==="
"$ZS" Square "$DOMAIN" >/tmp/xv_cyc_zs.log 2>&1 &
sleep 2.5
timeout 7 "$PY" "$HERE/shapes_pub.py" Square GREEN "$DOMAIN" >/tmp/xv_cyc_cp.log 2>&1
pkill -9 -f shapes_demo >/dev/null 2>&1; sleep 1
A=$(grep -c '<-' /tmp/xv_cyc_zs.log)
echo "ZeroDDS received from Cyclone: $A"; grep '<-' /tmp/xv_cyc_zs.log | head -2
sleep 1

echo "=== B) ZeroDDS pub -> CycloneDDS sub ==="
timeout 9 "$PY" "$HERE/shapes_sub.py" Square "$DOMAIN" >/tmp/xv_cyc_cs.log 2>&1 &
sleep 2.5
timeout 6 "$ZP" Square BLUE "$DOMAIN" >/tmp/xv_cyc_zp.log 2>&1
cleanup; sleep 1
B=$(grep -c '<-' /tmp/xv_cyc_cs.log)
echo "Cyclone received from ZeroDDS: $B"; grep '<-' /tmp/xv_cyc_cs.log | head -2

echo
if [ "$A" -gt 0 ] && [ "$B" -gt 0 ]; then echo "RESULT: PASS (both directions)"; else echo "RESULT: FAIL (A=$A B=$B)"; exit 1; fi
