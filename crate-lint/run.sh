#!/usr/bin/env sh
# The three `zerodds-lint` pragmas from the website's docs/lint.html, measured
# as a difference: the same code twice, once without the pragmas and once with
# them. A pragma that changes nothing is not demonstrated by a file that
# passes — only by the file that fails without it.
#
# ZERODDS_SRC is the zero-dds checkout (the harness exports it).
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
SRC="${ZERODDS_SRC:-$(cd "$HERE/../.." && pwd)}"

( cd "$SRC" && cargo build -q -p zerodds-lint --release )
LINT="${CARGO_TARGET_DIR:-$SRC/target}/release/zerodds-lint"

echo "== without the pragmas =="
set +e
"$LINT" check --root "$HERE/no-pragmas"
without=$?
set -e

echo
echo "== with the pragmas =="
"$LINT" check --root "$HERE/pragmas"
with=$?

echo
if [ "$without" -eq 0 ]; then
    echo "no-pragmas passed the lint; the fixture no longer triggers anything" >&2
    exit 1
fi
if [ "$with" -ne 0 ]; then
    echo "pragmas did not silence the lint" >&2
    exit 1
fi
echo "lint: 2 errors without the pragmas, 0 with them"
