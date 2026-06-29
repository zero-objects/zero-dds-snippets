#!/usr/bin/env bash
# Build + run the FEATURE-corpus XCDR2 interop harness (FeaturesInterop).
# Generates feat::{WStr,Mut,Bits,Tree,Arr,Prim} via zerodds-idlc --java, javac's
# them against the omgdds runtime jar, then ENCODE+DECODE against the goldens.
set -euo pipefail

REPO="${REPO:-/path/to/zerodds}"
HERE="$(cd "$(dirname "$0")" && pwd)"
JAVA_HOME="${JAVA_HOME:-$(/usr/libexec/java_home -v 21)}"
export JAVA_HOME
JAR="$REPO/crates/java-omgdds/java/target/omgdds-0.0.0.jar"

if [ ! -f "$JAR" ]; then
  ( cd "$REPO/crates/java-omgdds/java" && mvn -q -DskipTests install )
fi

# 1) Generate the feature types (fresh each run).
GEN="$HERE/features-generated"
rm -rf "$GEN" "$HERE/out-features"
mkdir -p "$GEN" "$HERE/out-features"
"$REPO/target/debug/zerodds-idlc" generate \
  "$REPO/zerodds-examples/idl-conformance/_interop/features.idl" \
  --java -o "$GEN" </dev/null

# 2) Compile generated types + the harness.
SRCS="$(mktemp)"
find "$GEN" -name '*.java' > "$SRCS"
find "$HERE/runtime-support" -name '*.java' >> "$SRCS"
echo "$HERE/FeaturesInterop.java" >> "$SRCS"
"$JAVA_HOME/bin/javac" -cp "$JAR" -d "$HERE/out-features" "@$SRCS"
rm -f "$SRCS"

# 3) Run.
"$JAVA_HOME/bin/java" -cp "$HERE/out-features:$JAR" FeaturesInterop "${1:-ROUNDTRIP}"
