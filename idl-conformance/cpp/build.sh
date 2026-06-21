#!/bin/sh
# Build the combo IDL-conformance roundtrip for the C++ binding.
# Run from this directory. Assumes the ZeroDDS checkout two levels up.
set -e

ZERODDS=/Users/sandrakessler/projects/zerodds

# (Re)generate the typed header from the conformance fixture.
#   "$ZERODDS/target/debug/zerodds-idlc" generate \
#       "$ZERODDS/tools/idlc/tests/conformance/fixtures/20_mixed_combo.idl" \
#       --cpp -o gen </dev/null
# The generated header in gen/ is checked in, so this is optional.

clang++ -std=c++17 combo_roundtrip.cpp \
  -I . \
  -I "$ZERODDS/crates/cpp/include" \
  -I "$ZERODDS/crates/zerodds-c-api/include" \
  -L "$ZERODDS/target/release" -lzerodds \
  -o combo_roundtrip

echo "built ./combo_roundtrip"
echo "run with: DYLD_LIBRARY_PATH=$ZERODDS/target/release ./combo_roundtrip"
