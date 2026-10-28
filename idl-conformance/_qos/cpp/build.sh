#!/usr/bin/env bash
# Generates gen/reading.hpp from reading.idl with zerodds-idlc, builds the QoS
# harness against the C++ binding (DDS-PSM-Cxx over the C ABI) and the native
# library, and runs it.
#
#   ZERODDS_SRC   the zero-dds checkout; default: the one this directory sits in
#   IDLC          the zerodds-idlc binary; built from ZERODDS_SRC when unset
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="${ZERODDS_SRC:-$(cd "$HERE/../../../.." && pwd)}"
TARGET="${CARGO_TARGET_DIR:-$REPO/target}"
( cd "$REPO" && cargo build -q -p zerodds-c-api )
if [ -z "${IDLC:-}" ]; then
    ( cd "$REPO" && cargo build -q -p zerodds-idlc )
    IDLC="$TARGET/debug/zerodds-idlc"
fi

cd "$HERE"
rm -rf gen
"$IDLC" generate --lang c++ --out gen reading.idl
make -s -C "$REPO/runtime/cpp" build/libzerodds_wire.a
c++ -std=c++17 -Wall qos_conformance.cpp -I . \
    -I "$REPO/crates/cpp/include" -I "$REPO/runtime/cpp/include" -I "$REPO/crates/zerodds-c-api/include" \
    "$REPO/runtime/cpp/build/libzerodds_wire.a" -L "$TARGET/debug" -lzerodds -o qos_conformance
LD_LIBRARY_PATH="$TARGET/debug${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
DYLD_LIBRARY_PATH="$TARGET/debug${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}" \
    ./qos_conformance
