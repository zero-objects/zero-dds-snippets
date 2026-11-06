#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# The C peer: generate its codec from idl/track.idl with zerodds-idlc,
# build it with runtime/c, and read the sample `cargo run -- FILE` wrote.
#
#     sh peer/run.sh
set -eu
here=$(cd "$(dirname "$0")" && pwd)
example=$(dirname "$here")
# The ZeroDDS source tree: $ZERODDS_SRC, or the tree that holds this
# repository (ci/run-examples.sh).
root=${ZERODDS_SRC:-$(cd "$example/../.." && pwd)}
out=${OUT:-"$example/target/peer"}
mkdir -p "$out"

cargo run --quiet --manifest-path "$example/Cargo.toml" -- "$out/sample.bin"
cargo run --quiet --manifest-path "$root/Cargo.toml" -p zerodds-idlc -- \
    generate --lang c --out "$out" "$example/idl/track.idl"
runtime="$root/runtime/c"
${CC:-cc} -std=c99 -Wall -Wno-unused-function \
    -I"$out" -I"$runtime/include" -o "$out/read_track" "$here/read_track.c" \
    "$runtime/src/writer.c" "$runtime/src/reader.c" "$runtime/src/float128.c" \
    "$runtime/src/float128_decimal.c" "$runtime/src/md5.c"
"$out/read_track" "$out/sample.bin"
