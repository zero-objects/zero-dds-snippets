#!/usr/bin/env bash
# zerodds-idlc dlrl: DDS 1.2 §8 DLRL object classes from #pragma
# DCPS_DATA_TYPE / DCPS_DATA_KEY / DCPS_DLRL_RELATION, in TypeScript, the
# one of the four languages whose output needs no DLRL runtime; checked with
# tsc --strict against application code that uses them, then run with node.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
: "${ZERODDS_SRC:?a zero-dds checkout}"
IDLC=${IDLC:-}
if [ -z "$IDLC" ]; then
  (cd "$ZERODDS_SRC" && cargo build -q --release -p zerodds-idlc)
  IDLC=${CARGO_TARGET_DIR:-$ZERODDS_SRC/target}/release/zerodds-idlc
fi
cd "$HERE"
rm -rf gen build
"$IDLC" dlrl --lang typescript --out gen trade.idl
tsc --strict --target es2020 --module commonjs --outDir build use.ts gen/trade_dlrl.ts
node build/use.js
