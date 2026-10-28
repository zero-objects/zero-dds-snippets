#!/usr/bin/env bash
# Cross-vendor proof: an OpenDDS publisher of `dyn::Track` (an @appendable
# struct carrying an @appendable union), decoded by `zerodds-spy` from the IDL
# alone — no compiled Rust type behind it. Prints how many samples of each
# union branch were decoded.
#
# Prereqs:
#   * zerodds-spy built:   cargo build --release -p zerodds-spy
#   * an OpenDDS source tree with RTPS, built, under DDS_ROOT (its setenv.sh
#     puts opendds_idl, ACE/TAO and the libraries on the paths), and cmake.
#     The publisher (pub.cpp, dyn.idl here) is built the first time.
#
# Env:
#   DDS_ROOT    the OpenDDS tree      (required)
#   DOMAIN      DDS domain id         (default 81)
#   SPY         path to zerodds-spy   (default $ZERODDS_SRC/target/release/zerodds-spy)
#
# The spy reads ../dyn_app.idl, the schema of the Cyclone and RTI legs.
# dyn.idl here is the same type with the key's must-understand flag written
# out for opendds_idl (see the comment in it).
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
DOMAIN="${DOMAIN:-81}"
# `ZERODDS_SRC` is what ci/run-examples.sh exports; outside it, the checkout is
# two levels up from this repository.
SRC="${ZERODDS_SRC:-$HERE/../../..}"
SPY="${SPY:-$SRC/target/release/zerodds-spy}"
BUILD="$HERE/build"

[ -x "$SPY" ] || { echo "missing $SPY — cargo build --release -p zerodds-spy"; exit 1; }
[ -f "${DDS_ROOT:-}/setenv.sh" ] || { echo "missing \$DDS_ROOT/setenv.sh — set DDS_ROOT to a built OpenDDS tree"; exit 1; }
# setenv.sh appends to LD_LIBRARY_PATH and PATH as it finds them, which
# `set -u` refuses when the first is unset.
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
# shellcheck disable=SC1091
. "$DDS_ROOT/setenv.sh"

# ---- the band this run will use has to be free -----------------------------
command -v ss >/dev/null 2>&1 || {
    echo "ABORT: ss(8) is not installed, so this host cannot say whether the" \
         "band of domain $DOMAIN is free; it will not measure into an unknown host"; exit 1; }
BASE=$((7400 + 250 * DOMAIN)); HI=$((BASE + 249))
BUSY="$(ss -uln 2>/dev/null | awk -v b="$BASE" -v h="$HI" \
    '{n=$5; sub(/.*:/,"",n); if (n+0>=b && n+0<=h) print n}' | sort -un)"
if [ -n "$BUSY" ]; then
    echo "ABORT: the RTPS band of domain $DOMAIN ($BASE-$HI) is in use:"; echo "$BUSY"
    exit 1
fi
echo "domain $DOMAIN free, band $BASE-$HI empty"

# ---- the OpenDDS publisher -------------------------------------------------
if [ ! -x "$BUILD/cmake/od_pub" ]; then
    echo "=== generating the OpenDDS type support and building the publisher ==="
    mkdir -p "$BUILD"
    cmake -S "$HERE" -B "$BUILD/cmake" -DOpenDDS_ROOT="$DDS_ROOT" \
        -DCMAKE_BUILD_TYPE=Release >"$BUILD/cmake.log" 2>&1 &&
    cmake --build "$BUILD/cmake" -j >"$BUILD/build.log" 2>&1 || {
        echo "cmake failed:"; tail -30 "$BUILD/cmake.log" "$BUILD/build.log"; exit 1; }
fi

LOG="$(mktemp -d)"
echo "=== OpenDDS pub -> zerodds-spy reflective decode (domain $DOMAIN) ==="
"$SPY" -d "$DOMAIN" -t TrackTopic --type-file "$HERE/../dyn_app.idl" --decode \
    --duration 25s >"$LOG/spy.log" 2>&1 &
SPYPID=$!
sleep 3
# The publisher writes 40 samples, one every 400 ms.
( cd "$HERE" && timeout 20 "$BUILD/cmake/od_pub" -DCPSConfigFile rtps.ini "$DOMAIN" ) \
    >"$LOG/op.log" 2>&1
wait "$SPYPID" 2>/dev/null

N=$(grep -c '"command"' "$LOG/spy.log")
SPEED=$(grep -c '"speed"' "$LOG/spy.log")
ANGLE=$(grep -c '"angle"' "$LOG/spy.log")
FLAG=$(grep -c '"flag"' "$LOG/spy.log")
echo "ZeroDDS reflectively decoded from OpenDDS: $N  (speed=$SPEED angle=$ANGLE flag=$FLAG)"
grep '"command"' "$LOG/spy.log" | head -3

echo
echo "logs: $LOG"
if [ "$N" -gt 0 ] && [ "$SPEED" -gt 0 ] && [ "$ANGLE" -gt 0 ] && [ "$FLAG" -gt 0 ]; then
    echo "RESULT: PASS ($N samples, all three union branches)"
else
    echo "RESULT: FAIL (n=$N speed=$SPEED angle=$ANGLE flag=$FLAG)"
    tail -20 "$LOG/spy.log" | sed 's/^/  spy| /'
    tail -10 "$LOG/op.log" | sed 's/^/  od| /'
    exit 1
fi
