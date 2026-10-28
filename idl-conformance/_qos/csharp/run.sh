#!/usr/bin/env bash
# Generates the C# Reading type, codec and TypeSupport from reading.idl with
# zerodds-idlc into generated/, then builds and runs the QoS harness against
# the native library in the checkout's target directory.
#
#   ZERODDS_SRC   the zero-dds checkout; default: the one this directory sits in
#   IDLC          the zerodds-idlc binary; built from ZERODDS_SRC when unset
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="${ZERODDS_SRC:-$(cd "$HERE/../../../.." && pwd)}"
TARGET="${CARGO_TARGET_DIR:-$REPO/target}"
if [ -z "${IDLC:-}" ]; then
    ( cd "$REPO" && cargo build -q -p zerodds-idlc )
    IDLC="$TARGET/debug/zerodds-idlc"
fi

cd "$HERE"
rm -rf generated
"$IDLC" generate --lang 'c#' --out generated reading.idl
dotnet run --project qos-csharp.csproj -p:NativeLibraryDirectory="$TARGET/debug"
