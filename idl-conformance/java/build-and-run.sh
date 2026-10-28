#!/usr/bin/env bash
# Fresh generated Java -> explicit DDS descriptor -> local DDS API loopback.
set -euo pipefail
#
# Every option has a default: --repo is the checkout this directory sits in
# (or $ZERODDS_SRC), --idlc is built from it, and --binding is $ZERODDS_JAR or
# the jar `mvn install` leaves in crates/java-omgdds/java/target.
usage() {
    echo "usage: $0 [--repo DIR] [--idlc EXECUTABLE] [--binding JAR_OR_CLASSES] [--domain INTEGER]" >&2
    exit 2
}
repo="" idlc="" binding="" domain=177
while [ "$#" -gt 0 ]; do
    [ "$#" -ge 2 ] || usage
    case "$1" in
        --repo) repo="$2" ;;
        --idlc) idlc="$2" ;;
        --binding) binding="$2" ;;
        --domain) domain="$2" ;;
        *) usage ;;
    esac
    shift 2
done
case "$domain" in ''|*[!0-9]*) usage ;; esac
[ "${#domain}" -le 3 ] && [ "$domain" -le 232 ] || usage
here="$(cd "$(dirname "$0")" && pwd)"
[ -n "$repo" ] || repo="${ZERODDS_SRC:-$here/../../..}"
repo="$(cd "$repo" && pwd)"
if [ -z "$idlc" ]; then
    ( cd "$repo" && cargo build -q -p zerodds-idlc )
    idlc="${CARGO_TARGET_DIR:-$repo/target}/debug/zerodds-idlc"
fi
[ -n "$binding" ] || binding="${ZERODDS_JAR:-$(ls "$repo"/crates/java-omgdds/java/target/omgdds-*.jar 2>/dev/null | head -n 1)}"
[ -n "$binding" ] || { echo "No Java binding jar: mvn -q install -DskipTests in $repo/crates/java-omgdds/java" >&2; exit 2; }
[ -x "$idlc" ] || { echo "CLI is not executable: $idlc" >&2; exit 2; }
idlc="$(cd "$(dirname "$idlc")" && pwd)/$(basename "$idlc")"
binding="$(cd "$(dirname "$binding")" && pwd)/$(basename "$binding")"
idl="$here/../rust/idl/20_mixed_combo.idl"
wire="$repo/runtime/java/src/main/java"
[ -f "$idl" ] && [ -d "$wire/org/zerodds/wire" ] || { echo "Missing shared IDL or wire runtime" >&2; exit 2; }
for tool in java javac jar; do command -v "$tool" >/dev/null || { echo "Missing JDK tool: $tool" >&2; exit 2; }; done
case "${ZERODDS_RTPS:-}" in 1|[Tt][Rr][Uu][Ee]) echo "Unset ZERODDS_RTPS for local loopback" >&2; exit 2 ;; esac
for entry in org/zerodds/cdr/TopicTypeSupport.class org/zerodds/rtps/SerializedSample.class org/omg/dds/sub/DataReader.class; do
    if [ -d "$binding" ]; then
        [ -f "$binding/$entry" ] || { echo "Missing binding class $entry" >&2; exit 2; }
    elif [ -f "$binding" ]; then
        jar tf "$binding" | grep -Fx "$entry" >/dev/null || { echo "Missing binding JAR entry $entry" >&2; exit 2; }
    else
        echo "Explicit binding JAR/classes path does not exist: $binding" >&2; exit 2
    fi
done
work="$(mktemp -d "${TMPDIR:-/tmp}/zerodds-java-example.XXXXXX")"
echo "Fresh outputs retained at: $work"
mkdir "$work/generated" "$work/classes"
"$idlc" check "$idl"
"$idlc" generate --lang java --encoding xcdr2 --out "$work/generated" "$idl"
sources=()
for name in ZdTypes ZdCodec ZdTypeSupports; do
    path="$work/generated/$name.java"
    [ -f "$path" ] || { echo "Missing fresh output: $path" >&2; exit 1; }
    sources+=("$path")
done
while IFS= read -r -d '' path; do sources+=("$path"); done < <(find "$wire/org/zerodds/wire" -name '*.java' -type f -print0)
# Historical generated/, runtime-support/ and out/ are never included.
javac -Werror -encoding UTF-8 -cp "$binding" -d "$work/classes" "${sources[@]}" "$here/App.java"
java -Dzerodds.rtps.enable=false -cp "$work/classes:$binding" App "$domain"
