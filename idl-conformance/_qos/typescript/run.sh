#!/usr/bin/env bash
# Generates gen/reading.ts from reading.idl with zerodds-idlc, type-checks the
# probe against @zerodds/node and @zerodds/wire, and runs the nine QoS and
# keyed-lifecycle checks.
#
#   ZERODDS_SRC   the zero-dds checkout; default: the one this directory sits in
#   IDLC          the zerodds-idlc binary; built from ZERODDS_SRC when unset
#
# `tsc` and `tsx` come from the PATH — crates/ts-node's node_modules/.bin
# carries both once that package is installed.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="${ZERODDS_SRC:-$(cd "$HERE/../../../.." && pwd)}"
if [ -z "${IDLC:-}" ]; then
    ( cd "$REPO" && cargo build -q -p zerodds-idlc )
    IDLC="${CARGO_TARGET_DIR:-$REPO/target}/debug/zerodds-idlc"
fi

cd "$HERE"
rm -rf gen
"$IDLC" generate --lang typescript --out gen reading.idl
tsc --noEmit -p tsconfig.json
tsx qos_conformance.ts
