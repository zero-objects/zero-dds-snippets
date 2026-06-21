#!/usr/bin/env bash
# Build the org.omg.dds Java PSM jar, compile + run the QoS conformance harness.
set -euo pipefail
REPO=/Users/sandrakessler/projects/zerodds
HERE="$(cd "$(dirname "$0")" && pwd)"
JAVA_HOME="${JAVA_HOME:-$(/usr/libexec/java_home -v 21 2>/dev/null || /usr/libexec/java_home)}"
export JAVA_HOME
JAR="$REPO/crates/java-omgdds/java/target/omgdds-0.0.0.jar"

( cd "$REPO/crates/java-omgdds/java" && mvn -q -DskipTests install )

rm -rf "$HERE/out"; mkdir -p "$HERE/out"
"$JAVA_HOME/bin/javac" -cp "$JAR" -d "$HERE/out" "$HERE/QosConformance.java"
"$JAVA_HOME/bin/java" -cp "$HERE/out:$JAR" QosConformance
