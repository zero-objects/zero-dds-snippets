#!/usr/bin/env bash
# Build + run the ZeroDDS Java IDL-conformance loopback example.
# Self-contained: builds the binding jar once, javac's the generated sources +
# the idl-java runtime support + App.java, then runs the roundtrip.
set -euo pipefail

REPO="${REPO:-/path/to/zerodds}"
HERE="$(cd "$(dirname "$0")" && pwd)"
JAVA_HOME="${JAVA_HOME:-$(/usr/libexec/java_home -v 21)}"
export JAVA_HOME
JAR="$REPO/crates/java-omgdds/java/target/omgdds-0.0.0.jar"

# 1) Build the binding jar once (writes only build artifacts).
if [ ! -f "$JAR" ]; then
  ( cd "$REPO/crates/java-omgdds/java" && mvn -q -DskipTests install )
fi

# 2) Compile generated types + idl-java runtime support + the App.
rm -rf "$HERE/out"
mkdir -p "$HERE/out"
SRCS="$(mktemp)"
find "$HERE/generated" -name '*.java' >  "$SRCS"
find "$HERE/runtime-support" -name '*.java' >> "$SRCS"
echo "$HERE/App.java" >> "$SRCS"
"$JAVA_HOME/bin/javac" -cp "$JAR" -d "$HERE/out" "@$SRCS"
rm -f "$SRCS"

# 3) Run.
"$JAVA_HOME/bin/java" -cp "$HERE/out:$JAR" App
