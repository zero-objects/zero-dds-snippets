#!/usr/bin/env bash
# Capture → transcode → replay loop using only the rc.4 CLI tools.
# Linux only (live discovery needs multicast loopback). Ctrl-C-safe.
set -euo pipefail
cd "$(dirname "$0")"
ROOT="$(cd ../.. && pwd)"

echo "▶ building record + replay + shapes demo…"
( cd "$ROOT" && cargo build -q -p zerodds-record -p zerodds-replay -p zerodds-dcps \
    --example shapes_demo_publisher --example shapes_demo_subscriber )
REC="$ROOT/target/debug/zerodds-record"
REPLAY="$ROOT/target/debug/zerodds-replay"

pids=(); cleanup(){ for p in "${pids[@]:-}"; do kill "$p" 2>/dev/null || true; done; }
trap cleanup EXIT INT TERM
run(){ ( cd "$ROOT" && "$@" ); }

echo "▶ recording Square (domain 0) → cap.zddsrec + cap.db + cap.ndjson (decode)"
"$REC" record -t Square -d 0 --duration 6s -o cap.zddsrec \
  --decode --type-file shapes.idl --map Square=ShapeType \
  --out-sqlite cap.db --out-json cap.ndjson >rec.log 2>&1 &
pids+=($!)

sleep 1
echo "▶ publisher on domain 0 (feeds the recorder)"
run timeout 5 cargo run -q -p zerodds-dcps --example shapes_demo_publisher -- Square BLUE 0 >pub.log 2>&1 || true
wait "${pids[0]}" 2>/dev/null || true; pids=()

echo
echo "▶ artifacts:"
for f in cap.zddsrec cap.db cap.ndjson; do [ -s "$f" ] && echo "    ✓ $f ($(wc -c <"$f" | tr -d ' ') B)" || { echo "    ✗ $f missing/empty"; exit 1; }; done

echo "▶ replaying from each persisted form onto domain 1"
ok=0
for art in cap.zddsrec cap.ndjson cap.db; do
  if "$REPLAY" replay "$art" --inject --inject-domain 1 >"replay-$art.log" 2>&1; then echo "    ✓ replay $art"; ok=$((ok+1)); else echo "    ✗ replay $art — see replay-$art.log"; fi
done
echo
[ "$ok" = 3 ] && echo "✓ PASS — captured once, replayed from all 3 formats" \
             || { echo "✗ only $ok/3 formats replayed"; exit 1; }
