#!/usr/bin/env bash
# Generates the Reading type from Reading.idl with zerodds-idlc, compiles the
# QoS harness against runtime/java and the org.omg.dds Java binding jar, and
# runs it.
#
#   ZERODDS_SRC   the zero-dds checkout; default: the one this directory sits in
#   IDLC          the zerodds-idlc binary; built from ZERODDS_SRC when unset
#   ZERODDS_JAR   the binding jar; default: the one `mvn install` leaves in
#                 crates/java-omgdds/java/target
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="${ZERODDS_SRC:-$(cd "$HERE/../../../.." && pwd)}"
if [ -z "${IDLC:-}" ]; then
    ( cd "$REPO" && cargo build -q -p zerodds-idlc )
    IDLC="${CARGO_TARGET_DIR:-$REPO/target}/debug/zerodds-idlc"
fi
JAR="${ZERODDS_JAR:-$(ls "$REPO"/crates/java-omgdds/java/target/omgdds-*.jar 2>/dev/null | head -n 1)}"
[ -n "$JAR" ] && [ -f "$JAR" ] || { echo "No Java binding jar: mvn -q install -DskipTests in $REPO/crates/java-omgdds/java" >&2; exit 2; }
WIRE="$REPO/runtime/java/src/main/java"

rm -rf "$HERE/gen" "$HERE/out"
mkdir -p "$HERE/gen" "$HERE/out"
"$IDLC" generate --lang java --out "$HERE/gen" "$HERE/Reading.idl"
sources=("$HERE/gen/ZdTypes.java" "$HERE/gen/ZdCodec.java" "$HERE/gen/ZdTypeSupports.java" "$HERE/QosConformance.java")
while IFS= read -r -d '' path; do sources+=("$path"); done < <(find "$WIRE/org/zerodds/wire" -name '*.java' -type f -print0)
javac -encoding UTF-8 -cp "$JAR" -d "$HERE/out" "${sources[@]}"
java -cp "$HERE/out:$JAR" QosConformance
