#!/usr/bin/env bash
# Cross-vendor proof: a Fast DDS publisher of `dyn::Track` (an @appendable
# struct carrying a @mutable union, ../dyn.idl), decoded by `zerodds-spy` from
# the IDL alone — no compiled Rust type behind it. Prints how many samples of
# each union branch were decoded.
#
# Prereqs:
#   * zerodds-spy built:   cargo build --release -p zerodds-spy
#   * Fast DDS 3 installed under FASTDDS_HOME, fastddsgen and cmake. The
#     publisher is generated from ../dyn.idl and built here the first time.
#
# Env:
#   FASTDDS_HOME  Fast DDS install    (default /opt/fastdds)
#   FASTDDSGEN    fastddsgen          (default: the one on PATH)
#   DOMAIN      DDS domain id         (default 80)
#   SPY         path to zerodds-spy   (default $ZERODDS_SRC/target/release/zerodds-spy)
#
# Fast DDS is the one peer whose IDL compiler accepts the @mutable union of
# ../dyn.idl, so this leg carries the mutable-union proof; the others measure
# the @appendable variant ../dyn_app.idl.
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
FASTDDS_HOME="${FASTDDS_HOME:-/opt/fastdds}"
FASTDDSGEN="${FASTDDSGEN:-fastddsgen}"
DOMAIN="${DOMAIN:-80}"
# `ZERODDS_SRC` is what ci/run-examples.sh exports; outside it, the checkout is
# two levels up from this repository.
SRC="${ZERODDS_SRC:-$HERE/../../..}"
SPY="${SPY:-$SRC/target/release/zerodds-spy}"
BUILD="$HERE/build"

[ -x "$SPY" ] || { echo "missing $SPY — cargo build --release -p zerodds-spy"; exit 1; }
command -v "$FASTDDSGEN" >/dev/null 2>&1 || { echo "missing fastddsgen — set FASTDDSGEN"; exit 1; }
[ -d "$FASTDDS_HOME/lib" ] || { echo "missing $FASTDDS_HOME/lib — set FASTDDS_HOME to a Fast DDS install"; exit 1; }
export LD_LIBRARY_PATH="$FASTDDS_HOME/lib:${LD_LIBRARY_PATH:-}"

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

# ---- the Fast DDS publisher ------------------------------------------------
if [ ! -x "$BUILD/bin/dyn_fd_pub" ]; then
    echo "=== generating the Fast DDS type support and building the publisher ==="
    mkdir -p "$BUILD/src"
    cp "$HERE/../dyn.idl" "$HERE/pub.cxx" "$HERE/CMakeLists.txt" "$BUILD/src/"
    ( cd "$BUILD/src" || exit 1
      "$FASTDDSGEN" -replace dyn.idl >gen.log 2>&1 || {
          echo "fastddsgen failed:"; tail -20 gen.log; exit 1; }
      cmake -S . -B ../cmake -DCMAKE_PREFIX_PATH="$FASTDDS_HOME" \
          -DCMAKE_BUILD_TYPE=Release >cmake.log 2>&1 &&
      cmake --build ../cmake -j >build.log 2>&1 || {
          echo "cmake failed:"; tail -30 cmake.log build.log; exit 1; }
      mkdir -p ../bin && cp ../cmake/dyn_fd_pub ../bin/
    ) || exit 1
fi

LOG="$(mktemp -d)"
echo "=== Fast DDS pub -> zerodds-spy reflective decode (domain $DOMAIN) ==="
# Fast DDS writes the @mutable union in XCDR1 PL_CDR with bare parameter
# ids, true lengths and a bare 0x3f02 sentinel; the spy reads that only when
# told to (--dialect fastdds) and reports it as a framing error otherwise.
"$SPY" -d "$DOMAIN" -t TrackTopic --type-file "$HERE/../dyn.idl" --decode \
    --dialect fastdds --duration 25s >"$LOG/spy.log" 2>&1 &
SPYPID=$!
sleep 3
# The publisher writes every 500 ms, 60 samples.
timeout 20 "$BUILD/bin/dyn_fd_pub" "$DOMAIN" >"$LOG/fp.log" 2>&1
wait "$SPYPID" 2>/dev/null

N=$(grep -c '"command"' "$LOG/spy.log")
SPEED=$(grep -c '"speed"' "$LOG/spy.log")
ANGLE=$(grep -c '"angle"' "$LOG/spy.log")
FLAG=$(grep -c '"flag"' "$LOG/spy.log")
echo "ZeroDDS reflectively decoded from Fast DDS: $N  (speed=$SPEED angle=$ANGLE flag=$FLAG)"
grep '"command"' "$LOG/spy.log" | head -3

echo
echo "logs: $LOG"
if [ "$N" -gt 0 ] && [ "$SPEED" -gt 0 ] && [ "$ANGLE" -gt 0 ] && [ "$FLAG" -gt 0 ]; then
    echo "RESULT: PASS ($N samples, all three union branches)"
else
    echo "RESULT: FAIL (n=$N speed=$SPEED angle=$ANGLE flag=$FLAG)"
    tail -20 "$LOG/spy.log" | sed 's/^/  spy| /'
    tail -10 "$LOG/fp.log" | sed 's/^/  fd| /'
    exit 1
fi
