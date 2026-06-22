#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
#
# Build the three secure XTypes peers (zerodds / fastdds / cyclone) from the
# shared secure_xtypes.idl. Designed for a vendor layout under /opt/*,
# overridable via env. Each peer is built only if its SDK is present.
#
#   REPO       ZeroDDS checkout (has target/release/libzerodds.so + crates/)
#   CYCLONE    /opt/cyclone     FASTDDS  /opt/fastdds
#   OUT        ./build (binaries land here)
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="${REPO:-$(cd "$HERE/../../../.." && pwd)}"
CYCLONE="${CYCLONE:-/opt/cyclone}"
FASTDDS="${FASTDDS:-/opt/fastdds}"
OUT="${OUT:-$HERE/build}"
IDL="$HERE/idl/secure_xtypes.idl"
FGEN="${FGEN:-$(command -v fastddsgen || echo "$FASTDDS/bin/fastddsgen")}"
mkdir -p "$OUT" "$HERE/gen/zerodds" "$HERE/gen/cyclone" "$HERE/gen/fastdds"

# ZeroDDS typed header: a known-good idl-cpp reference is checked in at
# gen/zerodds/secure_xtypes.hpp so the proof builds without a local idl-cpp
# toolchain. Regenerate only if explicitly asked (REGEN_ZERODDS=1) AND the local
# codegen produces a header that still references the nested type fully-qualified
# (older idl-cpp emitted unqualified `Inner` in the nested-sequence decode).
if [ "${REGEN_ZERODDS:-0}" = 1 ] && [ -d "$REPO/tools/dds-roundtrip-codegen" ]; then
  echo "== codegen: zerodds (idl-cpp) =="
  ( cd "$REPO/tools/dds-roundtrip-codegen" && cargo run -q -- "$IDL" --out "$HERE/gen/zerodds/secure_xtypes.hpp" )
fi

LIB="$REPO/target/release/libzerodds.so"
if [ -f "$LIB" ]; then
  echo "== build zerodds-secxt =="
  g++ -std=c++17 -O2 -o "$OUT/zerodds-secxt" "$HERE/apps/zerodds_secxt.cpp" \
    -I"$REPO/crates/zerodds-c-api/include" -I"$REPO/crates/cpp/include" -I"$HERE/gen/zerodds" \
    "$LIB" -lpthread -ldl -lm
else echo "!! libzerodds.so missing ($LIB) -- run: cargo build --release -p zerodds-c-api --features security"; fi

if [ -x "$CYCLONE/bin/idlc" ]; then
  echo "== codegen + build cyclone-secxt =="
  ( cd "$HERE/gen/cyclone" && "$CYCLONE/bin/idlc" -l cxx "$IDL" )
  g++ -std=c++17 -O2 -o "$OUT/cyclone-secxt" "$HERE/apps/cyclone_secxt.cpp" "$HERE/gen/cyclone/secure_xtypes.cpp" \
    -I"$CYCLONE/include" -I"$CYCLONE/include/ddscxx" -I"$HERE/gen/cyclone" \
    -L"$CYCLONE/lib" -lddscxx -lddsc -lpthread
else echo "!! cyclone idlc missing"; fi

if [ -x "$FGEN" ] || command -v fastddsgen >/dev/null; then
  echo "== codegen + build fastdds-secxt =="
  ( cd "$HERE/gen/fastdds" && "$FGEN" -replace "$IDL" >/dev/null )
  g++ -std=c++17 -O2 -o "$OUT/fastdds-secxt" "$HERE/apps/fastdds_secxt.cpp" \
    "$HERE/gen/fastdds/secure_xtypesPubSubTypes.cxx" \
    $( [ -f "$HERE/gen/fastdds/secure_xtypesTypeObjectSupport.cxx" ] && echo "$HERE/gen/fastdds/secure_xtypesTypeObjectSupport.cxx" ) \
    -I"$HERE/gen/fastdds" -I"$FASTDDS/include" -L"$FASTDDS/lib" -lfastdds -lfastcdr -lpthread
else echo "!! fastddsgen missing"; fi

echo "== built: =="; ls -la "$OUT"
