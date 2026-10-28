#!/usr/bin/env bash
# Generates the C# types, codecs and TypeSupport from conformance_combo.idl
# with zerodds-idlc into generated/, then builds and runs the round-trip
# against the native library in the checkout's target directory.
#
#   ZERODDS_SRC   the zero-dds checkout; default: the one this directory sits in
#   IDLC          the zerodds-idlc binary; built from ZERODDS_SRC when unset
#   DOMAIN        the DDS domain; default 177
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="${ZERODDS_SRC:-$(cd "$HERE/../../.." && pwd)}"
TARGET="${CARGO_TARGET_DIR:-$REPO/target}"
if [ -z "${IDLC:-}" ]; then
    ( cd "$REPO" && cargo build -q -p zerodds-idlc )
    IDLC="$TARGET/debug/zerodds-idlc"
fi

cd "$HERE"
rm -rf generated
"$IDLC" generate --lang 'c#' --encoding xcdr2 --out generated conformance_combo.idl
dotnet run --project csharp.csproj -p:NativeLibraryDirectory="$TARGET/debug" -- --domain "${DOMAIN:-177}"
