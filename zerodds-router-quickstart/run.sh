#!/usr/bin/env bash
# One-shot zerodds-router demo: bridge the Shapes demo from domain 0 → domain 1.
# Linux only (cross-domain discovery needs multicast loopback). Ctrl-C-safe.
set -euo pipefail
cd "$(dirname "$0")"
ROOT="$(cd ../.. && pwd)"
CONFIG="${1:-router.json}"   # pass router.xml to drive the XML form

echo "▶ building zerodds-router + shapes demo…"
( cd "$ROOT" && cargo build -q -p zerodds-routing-service -p zerodds-dcps \
    --bin zerodds-router --example shapes_demo_publisher --example shapes_demo_subscriber )

pids=()
cleanup() { for p in "${pids[@]:-}"; do kill "$p" 2>/dev/null || true; done; }
trap cleanup EXIT INT TERM

run() { ( cd "$ROOT" && "$@" ); }

echo "▶ subscriber on OUTPUT domain 1 (topic Square) → out.log"
run cargo run -q -p zerodds-dcps --example shapes_demo_subscriber -- Square 1 >out.log 2>&1 &
pids+=($!)

echo "▶ router: bridge domain 0 → domain 1 ($CONFIG)"
run cargo run -q -p zerodds-routing-service --bin zerodds-router -- run --config "$PWD/$CONFIG" >router.log 2>&1 &
pids+=($!)

sleep 2  # let discovery settle on both domains
echo "▶ publisher on INPUT domain 0 (topic Square) for 5s"
run timeout 5 cargo run -q -p zerodds-dcps --example shapes_demo_publisher -- Square BLUE 0 >pub.log 2>&1 || true

sleep 1
echo
if grep -q 'Square' out.log; then
  echo "✓ PASS — domain-1 subscriber received Square samples bridged from domain 0:"
  grep -m3 'Square' out.log | sed 's/^/    /'
else
  echo "✗ no samples on domain 1 — see out.log / router.log (multicast loopback on Linux?)"
  exit 1
fi
