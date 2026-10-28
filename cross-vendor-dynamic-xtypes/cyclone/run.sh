#!/usr/bin/env bash
# Cross-vendor proof: a Cyclone DDS publisher of `dyn::Track` (an @appendable
# struct carrying an @appendable union), decoded by `zerodds-spy` from the IDL
# alone — no compiled Rust type behind it. Prints how many samples of each
# union branch were decoded.
#
# Prereqs:
#   * zerodds-spy built:   cargo build --release -p zerodds-spy
#   * Cyclone DDS (C library, headers and idlc) under CYCLONE. The publisher
#     is generated from ../dyn_app.idl and built here the first time.
#
# Env:
#   CYCLONE     Cyclone DDS install   (default /opt/cyclone)
#   DOMAIN      DDS domain id         (default 79)
#   SPY         path to zerodds-spy   (default $ZERODDS_SRC/target/release/zerodds-spy)
#
# Cyclone's idlc rejects the @mutable union of ../dyn.idl ("Mutable unions are
# not supported yet"); ../dyn_app.idl is the @appendable variant every
# vendor's IDL compiler accepts, and the one this leg measures.
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
CYCLONE="${CYCLONE:-/opt/cyclone}"
DOMAIN="${DOMAIN:-79}"
# `ZERODDS_SRC` is what ci/run-examples.sh exports; outside it, the checkout is
# two levels up from this repository.
SRC="${ZERODDS_SRC:-$HERE/../../..}"
SPY="${SPY:-$SRC/target/release/zerodds-spy}"
BUILD="$HERE/build"

[ -x "$SPY" ] || { echo "missing $SPY — cargo build --release -p zerodds-spy"; exit 1; }
[ -x "$CYCLONE/bin/idlc" ] || { echo "missing $CYCLONE/bin/idlc — set CYCLONE to a Cyclone DDS install"; exit 1; }
export LD_LIBRARY_PATH="$CYCLONE/lib:${LD_LIBRARY_PATH:-}"

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

# ---- the Cyclone publisher -------------------------------------------------
# pub.c includes "dyn.h", so the IDL is compiled under the name dyn.idl.
if [ ! -x "$BUILD/cyc_dyn_pub" ]; then
    echo "=== generating the Cyclone type support and building the publisher ==="
    mkdir -p "$BUILD"
    cp "$HERE/../dyn_app.idl" "$BUILD/dyn.idl"
    ( cd "$BUILD" || exit 1
      "$CYCLONE/bin/idlc" dyn.idl >idlc.log 2>&1 || {
          echo "idlc failed:"; tail -20 idlc.log; exit 1; }
      cc -std=c11 -I"$CYCLONE/include" -I. "$HERE/pub.c" dyn.c \
          -L"$CYCLONE/lib" -lddsc -o cyc_dyn_pub >cc.log 2>&1 || {
          echo "cc failed:"; tail -30 cc.log; exit 1; }
    ) || exit 1
fi

LOG="$(mktemp -d)"
echo "=== Cyclone pub -> zerodds-spy reflective decode (domain $DOMAIN) ==="
"$SPY" -d "$DOMAIN" -t TrackTopic --type-file "$HERE/../dyn_app.idl" --decode \
    --duration 25s >"$LOG/spy.log" 2>&1 &
SPYPID=$!
sleep 3
# The publisher writes every 400 ms until it is stopped.
timeout 20 "$BUILD/cyc_dyn_pub" "$DOMAIN" >"$LOG/cp.log" 2>&1
wait "$SPYPID" 2>/dev/null

N=$(grep -c '"command"' "$LOG/spy.log")
SPEED=$(grep -c '"speed"' "$LOG/spy.log")
ANGLE=$(grep -c '"angle"' "$LOG/spy.log")
FLAG=$(grep -c '"flag"' "$LOG/spy.log")
echo "ZeroDDS reflectively decoded from Cyclone: $N  (speed=$SPEED angle=$ANGLE flag=$FLAG)"
grep '"command"' "$LOG/spy.log" | head -3

echo
echo "logs: $LOG"
if [ "$N" -gt 0 ] && [ "$SPEED" -gt 0 ] && [ "$ANGLE" -gt 0 ] && [ "$FLAG" -gt 0 ]; then
    echo "RESULT: PASS ($N samples, all three union branches)"
else
    echo "RESULT: FAIL (n=$N speed=$SPEED angle=$ANGLE flag=$FLAG)"
    tail -20 "$LOG/spy.log" | sed 's/^/  spy| /'
    tail -10 "$LOG/cp.log" | sed 's/^/  cyc| /'
    exit 1
fi
