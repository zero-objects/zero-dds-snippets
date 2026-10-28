#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# native-frontends/go: the Go endpoint SDK end-to-end against a real ZeroDDS
# peer. Runs the idlc-typed ping-pong (sync + async) and the reliable-stream
# loss-recovery demo, and asserts each.
#
#   run.sh [base-port]
#
# Needs a checked-out ZeroDDS source tree (for endpoints/go + the peer binary).
# Set ZERODDS_ROOT, or place this snippets repo next to a `zero-dds` checkout.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
if [ -z "${ZERODDS_ROOT:-}" ]; then
    for c in "$HERE/../../.." "$HERE/../../../.." "$HERE/../../../zero-dds" "$HERE/../../../../zero-dds"; do
        if [ -d "$c/endpoints/go" ]; then ZERODDS_ROOT=$(cd "$c" && pwd); break; fi
    done
fi
if [ ! -d "${ZERODDS_ROOT:-/none}/endpoints/go" ]; then
    echo "error: set ZERODDS_ROOT to a zero-dds checkout (with endpoints/go)"; exit 1
fi
export ZERODDS_ROOT
PORT=${1:-17520}
GO_SDK="$ZERODDS_ROOT/endpoints/go"
IDL="$HERE/../ping.idl"

# The peer binary uses the current common wire and XRCE implementations.
export CARGO_BUILD_JOBS="${CARGO_BUILD_JOBS:-1}"
( cd "$ZERODDS_ROOT" && cargo build -q -p zerodds-endpoint-e2e --bin peer )
PEER="$ZERODDS_ROOT/target/debug/peer"

# Generate into a separate package without rewriting compiler output. Neither
# generated codecs nor go.mod files overwrite the checked-in example sources.
( cd "$ZERODDS_ROOT" && cargo build -q -p zerodds-idlc )
IDLC="$ZERODDS_ROOT/target/debug/zerodds-idlc"
GO_WORK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/zerodds-native-go.XXXXXX")
PP=""
cleanup() {
    if [ -n "$PP" ]; then
        kill "$PP" 2>/dev/null || true
        wait "$PP" 2>/dev/null || true
    fi
    # Retain generated sources and complete logs for reproducibility.
    printf 'Example evidence: %s\n' "$GO_WORK_DIR"
}
trap cleanup EXIT
mkdir -p "$GO_WORK_DIR/pingpong/generated" "$GO_WORK_DIR/reliable"
cp "$HERE/pingpong/main.go" "$GO_WORK_DIR/pingpong/main.go"
cp "$HERE/reliable/main.go" "$GO_WORK_DIR/reliable/main.go"
"$IDLC" generate --lang go --encoding xcdr2 --out "$GO_WORK_DIR/pingpong/generated" "$IDL"

for app in pingpong reliable; do
    (
        cd "$GO_WORK_DIR/$app"
        go mod init "$app"
        go mod edit -require=zeroddsendpoint@v0.0.0 -replace="zeroddsendpoint=$GO_SDK"
        go mod edit -require=zdwire@v0.0.0 -replace="zdwire=$ZERODDS_ROOT/runtime/go"
        if [ "$(uname -s)" = Darwin ]; then
            # The installed macOS loader refuses internal-linker binaries
            # without LC_UUID. Let the platform linker produce the Mach-O.
            CGO_ENABLED=1 GOMAXPROCS=2 go build -p 1 -ldflags=-linkmode=external -o app .
            # Sign this freshly linked local executable, not an OS policy
            # override. Older Go linkers can leave an invalid ad-hoc signature.
            codesign --force --sign - app
            codesign --verify app
        else
            GOMAXPROCS=2 go build -p 1 -o app .
        fi
    )
done

run_pingpong() {
    echo "== ping-pong ($1) =="
    "$PEER" pingpong "$PORT" >"$GO_WORK_DIR/peer-$1.log" 2>&1 & PP=$!
    sleep 0.3   # let the peer bind before the app sends
    "$GO_WORK_DIR/pingpong/app" "$1" "$PORT" | tee "$GO_WORK_DIR/pingpong-$1.log"
    wait "$PP"
    PP=""
    cat "$GO_WORK_DIR/peer-$1.log"
}

run_pingpong sync
run_pingpong async

echo "== reliable (loss recovery, drop every 3rd of 12) =="
N=12
"$PEER" reliable "$PORT" 3 "$N" >"$GO_WORK_DIR/peer-reliable.log" 2>&1 & PP=$!
sleep 0.3
"$GO_WORK_DIR/reliable/app" "$PORT" "$N" | tee "$GO_WORK_DIR/reliable.log"
wait "$PP"
PP=""
cat "$GO_WORK_DIR/peer-reliable.log"

echo "OK: ping-pong (sync+async) + reliable (go <-> Rust peer)"
