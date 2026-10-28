#!/usr/bin/env bash
# The Java column of _interop: generates the types of one IDL file with
# `zerodds-idlc generate --lang java`, compiles them with runtime/java and the
# driver class, and runs the driver with the remaining arguments.
#
#   ./build.sh FeaturesInterop ../features.idl ENCODE|DECODE|ROUNDTRIP
#   ./build.sh Interop ../combo.idl ENCODE|DECODE <file>
#
#   REPO      the zero-dds checkout; default: the one this directory sits in
#   IDLC      the zerodds-idlc binary; default: $REPO/target/debug/zerodds-idlc
#   BINDING   the OMG DDS Java binding jar (org.zerodds.cdr); default: the one
#             `mvn install` builds in $REPO/crates/java-omgdds/java/target
set -euo pipefail

[ $# -ge 2 ] || { echo "usage: $0 <driver class> <idl> [driver args...]" >&2; exit 2; }
MAIN="$1"
IDL="$2"
shift 2

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="${REPO:-$(cd "$HERE/../../../.." && pwd)}"
IDLC="${IDLC:-${CARGO_TARGET_DIR:-$REPO/target}/debug/zerodds-idlc}"
BINDING="${BINDING:-$(ls "$REPO"/crates/java-omgdds/java/target/omgdds-*.jar 2>/dev/null | head -n 1)}"
WIRE="$REPO/runtime/java/src/main/java"

[ -x "$IDLC" ] || { echo "zerodds-idlc is not built: $IDLC" >&2; exit 2; }
[ -n "$BINDING" ] && [ -f "$BINDING" ] || {
    echo "the Java binding jar is not built (mvn -q install -DskipTests in $REPO/crates/java-omgdds/java)" >&2
    exit 2
}
for tool in java javac; do command -v "$tool" >/dev/null || { echo "missing JDK tool: $tool" >&2; exit 2; }; done

# One generated tree and one class tree per driver: the generator emits the
# top-level classes ZdTypes, ZdCodec and ZdTypeSupports for every IDL file.
GEN="$HERE/generated-$MAIN"
OUT="$HERE/out-$MAIN"
rm -rf "$GEN" "$OUT"
mkdir -p "$GEN" "$OUT"
"$IDLC" generate --lang java --out "$GEN" "$HERE/$IDL"

sources=("$GEN/ZdTypes.java" "$GEN/ZdCodec.java" "$GEN/ZdTypeSupports.java" "$HERE/$MAIN.java")
while IFS= read -r -d '' path; do sources+=("$path"); done < <(find "$WIRE/org/zerodds/wire" -name '*.java' -type f -print0)
javac -encoding UTF-8 -cp "$BINDING" -d "$OUT" "${sources[@]}"

cd "$HERE"
java -cp "$OUT:$BINDING" "$MAIN" "$@"
