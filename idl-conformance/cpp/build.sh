#!/usr/bin/env bash
# Generates gen/combo.hpp from combo.idl with zerodds-idlc, builds the
# roundtrip against the C++ binding and the native library, and runs it.
#
#   ZERODDS_SRC   the zero-dds checkout; default: the one this directory sits in
#   IDLC          the zerodds-idlc binary; built from ZERODDS_SRC when unset
#   DOMAIN        the DDS domain; default 177
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="${ZERODDS_SRC:-$(cd "$HERE/../../.." && pwd)}"
TARGET="${CARGO_TARGET_DIR:-$REPO/target}"
( cd "$REPO" && cargo build -q -p zerodds-c-api )
if [ -z "${IDLC:-}" ]; then
    ( cd "$REPO" && cargo build -q -p zerodds-idlc )
    IDLC="$TARGET/debug/zerodds-idlc"
fi

cd "$HERE"
rm -rf gen
"$IDLC" generate --lang c++ --out gen combo.idl
make -s -C "$REPO/runtime/cpp" build/libzerodds_wire.a
c++ -std=c++17 -Wall -Werror combo_roundtrip.cpp -I . \
    -I "$REPO/crates/cpp/include" -I "$REPO/runtime/cpp/include" -I "$REPO/crates/zerodds-c-api/include" \
    "$REPO/runtime/cpp/build/libzerodds_wire.a" -L "$TARGET/debug" -lzerodds -o combo_roundtrip
LD_LIBRARY_PATH="$TARGET/debug${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
DYLD_LIBRARY_PATH="$TARGET/debug${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}" \
    ./combo_roundtrip "${DOMAIN:-177}"
