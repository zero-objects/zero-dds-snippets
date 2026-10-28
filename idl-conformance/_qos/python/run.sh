#!/usr/bin/env bash
# Generates reading.py from reading.idl with zerodds-idlc and runs the QoS
# and keyed-lifecycle checks against the installed zerodds Python binding.
#
#   ZERODDS_SRC   the zero-dds checkout; default: the one this directory sits in
#   IDLC          the zerodds-idlc binary; built from ZERODDS_SRC when unset
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="${ZERODDS_SRC:-$(cd "$HERE/../../../.." && pwd)}"
if [ -z "${IDLC:-}" ]; then
    ( cd "$REPO" && cargo build -q -p zerodds-idlc )
    IDLC="${CARGO_TARGET_DIR:-$REPO/target}/debug/zerodds-idlc"
fi

"$IDLC" generate --lang python --out "$HERE" "$HERE/reading.idl"
cd "$HERE"
python3 qos_conformance.py
