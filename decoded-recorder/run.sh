#!/usr/bin/env bash
# The decoded-recorder workflow on one host, from sensor.idl alone: a ZeroDDS
# publisher of telemetry::Reading (publisher/, generated from the same IDL)
# writes COUNT samples; zerodds-spy --decode prints them as JSON while
# zerodds-record --decode captures them into .zddsrec, NDJSON and SQLite; and
# zerodds-replay plays each of the three files back. Every step has to account
# for every sample.
#
# Prereqs:
#   * the tools built:     cargo build --release -p zerodds-spy -p zerodds-record -p zerodds-replay
#   * the publisher built: (cd publisher && cargo build --release)
#   * python3 (its sqlite3 module counts the SQLite rows)
#
# Env:
#   DOMAIN      DDS domain id        (default 85)
#   COUNT       samples written      (default 20)
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
DOMAIN="${DOMAIN:-85}"
COUNT="${COUNT:-20}"
# `ZERODDS_SRC` is what ci/run-examples.sh exports; outside it, the checkout is
# one level up from this repository.
SRC="${ZERODDS_SRC:-$HERE/../..}"
TOOLS="${CARGO_TARGET_DIR:-$SRC/target}/release"
PUB="${CARGO_TARGET_DIR:-$HERE/publisher/target}/release/reading_pub"
IDL="$HERE/sensor.idl"

for b in "$TOOLS/zerodds-spy" "$TOOLS/zerodds-record" "$TOOLS/zerodds-replay"; do
    [ -x "$b" ] || { echo "missing $b — cargo build --release -p zerodds-spy -p zerodds-record -p zerodds-replay"; exit 1; }
done
[ -x "$PUB" ] || { echo "missing $PUB — (cd publisher && cargo build --release)"; exit 1; }

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

LOG="$(mktemp -d)"
PIDS=""
cleanup() {
    for p in $PIDS; do kill "$p" 2>/dev/null; done
    sleep 1
    for p in $PIDS; do kill -9 "$p" 2>/dev/null; wait "$p" 2>/dev/null; done
    PIDS=""
}
trap cleanup EXIT

echo "=== spy and record on Readings, publisher writes $COUNT (domain $DOMAIN) ==="
"$TOOLS/zerodds-spy" -d "$DOMAIN" -t Readings --type-file "$IDL" --decode \
    --duration 12s >"$LOG/spy.log" 2>&1 &
SPYPID=$!; PIDS="$PIDS $SPYPID"
"$TOOLS/zerodds-record" record -d "$DOMAIN" -t Readings --decode --type-file "$IDL" \
    -o "$LOG/readings.zddsrec" --out-json "$LOG/readings.ndjson" \
    --out-sqlite "$LOG/readings.db" --duration 12s >"$LOG/record.log" 2>&1 &
RECPID=$!; PIDS="$PIDS $RECPID"
sleep 2
timeout 12 "$PUB" "$DOMAIN" "$COUNT" >"$LOG/pub.log" 2>&1
PUBRC=$?
wait "$SPYPID" 2>/dev/null
wait "$RECPID" 2>/dev/null
PIDS=""

SPY_N=$(grep -c '"sensor_id"' "$LOG/spy.log")
JSON_N=$(grep -c '"sensor_id"' "$LOG/readings.ndjson" 2>/dev/null)
SQL_N=$(python3 -c 'import sqlite3, sys
print(sqlite3.connect(sys.argv[1]).execute("SELECT count(*) FROM t_Readings").fetchone()[0])' \
    "$LOG/readings.db" 2>"$LOG/sqlite.err")
echo "publisher exit $PUBRC; spy decoded $SPY_N, NDJSON $JSON_N, SQLite ${SQL_N:-?}"
grep '"sensor_id"' "$LOG/spy.log" | head -2
head -1 "$LOG/readings.ndjson" 2>/dev/null

echo "=== replay of each file ==="
replayed() { # file -> the count zerodds-replay says it emitted
    "$TOOLS/zerodds-replay" replay "$1" --time-scale 20 >"$LOG/replay.out" 2>"$LOG/replay.err"
    sed -n 's/^replay: \([0-9]*\) \(frames\|samples\) emitted$/\1/p' "$LOG/replay.err"
}
R_REC=$(replayed "$LOG/readings.zddsrec")
R_JSON=$(replayed "$LOG/readings.ndjson")
R_SQL=$(replayed "$LOG/readings.db")
# A .zddsrec holds frames, and a frame is a sample or a change of an
# instance's state: the publisher's writer unregisters its instances when it
# is deleted, and the capture keeps that. NDJSON and SQLite hold the samples
# alone. The samples of the capture are its frames of kind Alive, and the
# replay emits every frame.
"$TOOLS/zerodds-replay" dump "$LOG/readings.zddsrec" >"$LOG/dump.csv" 2>/dev/null
REC_ALIVE=$(grep -c ',Alive,' "$LOG/dump.csv")
REC_STATE=$(grep -c ',NotAlive' "$LOG/dump.csv")
echo "replayed: .zddsrec ${R_REC:-?} frames (${REC_ALIVE} samples, ${REC_STATE} state changes), NDJSON ${R_JSON:-?}, SQLite ${R_SQL:-?}"

echo
echo "logs: $LOG"
FAILED=""
[ "$PUBRC" = 0 ] || FAILED="$FAILED publisher-exit=$PUBRC"
for pair in "spy:$SPY_N" "ndjson:$JSON_N" "sqlite:${SQL_N:-}" \
            "zddsrec-samples:${REC_ALIVE:-}" "replay-ndjson:${R_JSON:-}" "replay-sqlite:${R_SQL:-}"; do
    [ "${pair#*:}" = "$COUNT" ] || FAILED="$FAILED ${pair%%:*}=${pair#*:}"
done
[ "${R_REC:-}" = "$((REC_ALIVE + REC_STATE))" ] || \
    FAILED="$FAILED replay-zddsrec=${R_REC:-} (the capture has $((REC_ALIVE + REC_STATE)) frames)"
if [ -z "$FAILED" ]; then
    echo "RESULT: PASS ($COUNT samples decoded live, recorded three ways, replayed from each)"
else
    echo "RESULT: FAIL —$FAILED (want $COUNT each)"
    tail -5 "$LOG/pub.log" "$LOG/record.log" "$LOG/sqlite.err" "$LOG/replay.err" 2>/dev/null | sed 's/^/  | /'
    exit 1
fi
