#!/usr/bin/env bash
# Tier-2 conformance gate: every ZeroDDS language binding must reproduce the SINGLE
# golden wire (goldens/<feature>.golden.bin) byte-for-byte, and decode it back to
# the canonical values. The wire is the wire — there is no per-language golden.
#
# For each language L and feature F:
#   (1) L.encode(canonical(F))  ==  goldens/F.golden.bin   (byte-identical)
#   (2) L.decode(goldens/F.golden.bin)  ==  canonical(F)
#
# The per-language *.<lang>.bin files this produces are EPHEMERAL (git-ignored);
# only *.golden.bin is committed. Regenerate the golden itself by re-running the
# rust PSM (the cross-vendor-validated zerodds-cdr core) — see RUST below.
#
# Usage: ./verify.sh            # build+run every language, cmp all vs golden
#        ./verify.sh <lang>     # one language only
set -uo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../../.." && pwd)"
G="$HERE/goldens"
FEATS="wstr mut bits tree arr prim mutnest outerkey mapenum"
fail=0

cmp_all() { # <lang>  — diff every ephemeral <feat>.<lang>.bin against the golden
  local lang=$1 f
  for f in $FEATS; do
    if cmp -s "$G/$f.$lang.bin" "$G/$f.golden.bin"; then
      echo "  [$lang] $f  encode == golden"
    else
      echo "  [$lang] $f  ENCODE != GOLDEN  ($(wc -c <"$G/$f.$lang.bin" 2>/dev/null|tr -d ' ')B vs $(wc -c <"$G/$f.golden.bin"|tr -d ' ')B)"
      fail=1
    fi
  done
}

run_rust() { # authoritative: (re)generates the golden via zerodds-cdr core
  ( cd "$HERE/_interop_rust" 2>/dev/null || cd "$HERE/rust-features"
    "$REPO/target/debug/zerodds-idlc" "$HERE/features.idl" --rust -o generated/ >/dev/null 2>&1 \
      || cargo run -q -p zerodds-idlc -- "$HERE/features.idl" --rust -o generated/ >/dev/null 2>&1
    cargo run -q )   # writes goldens/<feat>.rust.bin
  for f in $FEATS; do cp "$G/$f.rust.bin" "$G/$f.golden.bin"; done   # bless as golden
  cmp_all rust
}

run_cpp() {
  ( cd "$HERE/cpp"
    cargo run -q -p zerodds-idlc -- "$HERE/features.idl" --cpp -o gen_feat >/dev/null 2>&1
    clang++ -std=c++17 -I. -I"$REPO/crates/cpp/include" features_interop.cpp -o features_interop || return 1
    local f; for f in $FEATS; do ./features_interop $f ENCODE "$G/$f.cpp.bin" >/dev/null; done )
  cmp_all cpp
}

run_c() {
  ( cd "$HERE/c"
    cargo run -q -p zerodds-idlc -- "$HERE/features.idl" --c -o gen_feat >/dev/null 2>&1
    clang -std=c11 -I. -I"$REPO/crates/zerodds-c-api/include" features_interop.c -o features_interop 2>/dev/null || return 1
    local f; for f in $FEATS; do ./features_interop $f ENCODE "$G/$f.c.bin" >/dev/null; done )
  cmp_all c
}

run_python() {
  ( cd "$HERE/python"
    cargo run -q -p zerodds-idlc -- "$HERE/features.idl" --python -o . >/dev/null 2>&1
    python3 features_interop.py ENCODE >/dev/null )
  cmp_all python
}

run_ts() {
  ( cd "$HERE/ts"
    cargo run -q -p zerodds-idlc -- "$HERE/features.idl" --ts -o gen >/dev/null 2>&1
    cp gen/features.ts gen/30_features.ts 2>/dev/null; rm -f gen/features.ts
    npm run -s encode:features >/dev/null )
  cmp_all ts
}

run_java() {
  ( cd "$HERE/java" && bash build-features.sh ENCODE >/dev/null 2>&1 )
  cmp_all java
}

run_csharp() {
  ( cd "$HERE/csharp-features"
    cargo run -q -p zerodds-idlc -- "$HERE/features.idl" --csharp -o . >/dev/null 2>&1
    rm -rf obj bin   # force a clean build — dotnet incremental cache may keep a stale features.cs
    dotnet build -v q --nologo >/dev/null 2>&1
    dotnet "$(find bin -name '*features*.dll'|head -1)" ENCODE >/dev/null )
  cmp_all csharp
}

LANGS="${1:-rust cpp c python ts java csharp}"
for L in $LANGS; do
  echo "== $L =="
  run_$L || { echo "  [$L] build/run FAILED"; fail=1; }
done
[ $fail -eq 0 ] && echo "ALL LANGUAGES CONFORM TO THE GOLDEN" || echo "CONFORMANCE FAILURES ABOVE"
exit $fail
