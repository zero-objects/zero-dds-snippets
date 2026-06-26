#!/usr/bin/env bash
# Cross-vendor proof: ZeroDDS <-> Fast DDS on the ShapesDemo `Square` topic.
# Direction A (Fast DDS pub -> ZeroDDS sub) is the verified path; direction B is
# attempted and reported (see README "Known issue").
#
# Prereqs:
#   * ZeroDDS shapes examples built (see ../README.md).
#   * The Fast DDS `topic_instances` example built with the @final ShapeType +
#     QoS tweaks from this directory's README.
#
# Env:
#   FASTDDS_BIN   path to the built `topic_instances` binary   (required)
#   FASTDDS_LIB   Fast DDS lib dir for LD_LIBRARY_PATH          (required)
#   ZD_TARGET     cargo target/examples dir (default ../../../target/debug/examples)
#   DOMAIN        domain id (default 0)
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
ZD="${ZD_TARGET:-$HERE/../../../target/debug/examples}"
DOMAIN="${DOMAIN:-0}"
: "${FASTDDS_BIN:?set FASTDDS_BIN to the built topic_instances binary}"
: "${FASTDDS_LIB:?set FASTDDS_LIB to the Fast DDS lib dir}"
export LD_LIBRARY_PATH="$FASTDDS_LIB:${LD_LIBRARY_PATH:-}"

ZS="$ZD/shapes_demo_subscriber"; ZP="$ZD/shapes_demo_publisher"
cleanup() { pkill -9 -f topic_instances >/dev/null 2>&1; pkill -9 -f shapes_demo >/dev/null 2>&1; }
trap cleanup EXIT
cleanup; sleep 1

echo "=== A) Fast DDS pub -> ZeroDDS sub ==="
"$ZS" Square "$DOMAIN" >/tmp/xv_fd_zs.log 2>&1 &
sleep 2.5
timeout 8 "$FASTDDS_BIN" publisher -d "$DOMAIN" -n Square -i 1 -s 50 >/tmp/xv_fd_fp.log 2>&1
pkill -9 -f shapes_demo >/dev/null 2>&1; sleep 1
A=$(grep -c '<-' /tmp/xv_fd_zs.log)
echo "ZeroDDS received from Fast DDS: $A"; grep '<-' /tmp/xv_fd_zs.log | head -2
sleep 1

echo "=== B) ZeroDDS pub -> Fast DDS sub ==="
timeout 11 "$FASTDDS_BIN" subscriber -d "$DOMAIN" -n Square -i 1 -s 40 >/tmp/xv_fd_fs.log 2>&1 &
sleep 2.5
timeout 8 "$ZP" Square BLUE "$DOMAIN" >/tmp/xv_fd_zp.log 2>&1
cleanup; sleep 1
B=$(grep -c 'RECEIVED' /tmp/xv_fd_fs.log)
echo "Fast DDS received from ZeroDDS: $B"; grep 'RECEIVED' /tmp/xv_fd_fs.log | head -2

echo
if [ "$A" -gt 0 ] && [ "$B" -gt 0 ]; then echo "RESULT: PASS (both directions)"; else echo "RESULT: FAIL (A=$A B=$B)"; exit 1; fi
