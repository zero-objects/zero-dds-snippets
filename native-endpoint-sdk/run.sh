#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# native-endpoint-sdk: end-to-end demo of the ZeroDDS native endpoint SDKs
# (ADR 0013), both directions. C / Python / Java endpoints publish a
# SensorReading to a real zerodds-xrce hub over UDP and receive a sample the
# hub pushes. Asserts all six exchanges.
#
#   run.sh [base-port]
#
# Needs a checked-out ZeroDDS source tree (for the endpoint SDK sources + the
# hub). Set ZERODDS_ROOT, or place this snippets repo next to a `zero-dds`
# checkout, or run from inside a zero-dds tree.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
if [ -z "${ZERODDS_ROOT:-}" ]; then
    for c in "$HERE/../.." "$HERE/../../.." "$HERE/../../zero-dds" "$HERE/../../../zero-dds"; do
        if [ -d "$c/endpoints" ]; then ZERODDS_ROOT=$(cd "$c" && pwd); break; fi
    done
fi
if [ ! -d "${ZERODDS_ROOT:-/none}/endpoints" ]; then
    echo "error: set ZERODDS_ROOT to a zero-dds checkout (with endpoints/)"; exit 1
fi
export ZERODDS_ROOT
PORT=${1:-17490}
E="$ZERODDS_ROOT/endpoints"
HUB="$ZERODDS_ROOT/target/debug/zerodds-xrce-agent-demo"
TO=$(command -v timeout || command -v gtimeout || true)
CINC="-I$E/c/include -I$E/c/test"
CSDK="$E/c/src/zerodds_wire.c $E/c/src/zerodds_endpoint.c $E/c/test/sample_sensor.c"

echo "== building hub + endpoints (ZERODDS_ROOT=$ZERODDS_ROOT) =="
( cd "$ZERODDS_ROOT" && cargo build -q -p zerodds-xrce-agent-demo )
gcc -O2 $CINC $CSDK "$HERE/c/publish.c" -o /tmp/nesdk_c_pub
gcc -O2 $CINC $CSDK "$HERE/c/receive.c" -o /tmp/nesdk_c_recv
javac -d /tmp/nesdk_java "$E/java/Zdw.java" "$E/java/ZdwEndpoint.java" \
    "$HERE/java/PublishUdp.java" "$HERE/java/ReceiveUdp.java"

echo ""; echo "== PUBLISH (endpoint -> hub): C / Python / Java =="
$TO ${TO:+20} "$HUB" recv "$PORT" 3 > /tmp/nesdk_hub.out 2>&1 &
HUB_PID=$!
sleep 1
/tmp/nesdk_c_pub 127.0.0.1 "$PORT"
python3 "$HERE/python/publish.py" 127.0.0.1 "$PORT"
java -cp /tmp/nesdk_java PublishUdp 127.0.0.1 "$PORT"
wait "$HUB_PID" 2>/dev/null || true
cat /tmp/nesdk_hub.out
PUB_OK=$(grep -c "AGENT OK" /tmp/nesdk_hub.out || true)

echo ""; echo "== RECEIVE (hub -> endpoint): C / Python / Java =="
RECV_OK=0
recv_one() {
    local out="/tmp/nesdk_recv_$1.out"
    $TO ${TO:+10} bash -c "$2" > "$out" 2>&1 &
    local pid=$!
    sleep 1
    "$HUB" send 127.0.0.1 "$3" >/dev/null
    wait "$pid" 2>/dev/null || true
    cat "$out"
    grep -q "RECEIVER OK" "$out" && RECV_OK=$((RECV_OK + 1))
}
recv_one c "/tmp/nesdk_c_recv $((PORT+1))" $((PORT+1))
recv_one py "ZERODDS_ROOT='$ZERODDS_ROOT' python3 '$HERE/python/receive.py' $((PORT+2))" $((PORT+2))
recv_one java "java -cp /tmp/nesdk_java ReceiveUdp $((PORT+3))" $((PORT+3))

echo ""
if [ "$PUB_OK" -eq 3 ] && [ "$RECV_OK" -eq 3 ]; then
    echo "OK: publish 3/3 + receive 3/3 (C/Python/Java <-> Rust hub)"
else
    echo "FAILED: publish $PUB_OK/3, receive $RECV_OK/3"; exit 1
fi
