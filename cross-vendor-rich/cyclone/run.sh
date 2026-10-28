#!/usr/bin/env bash
# Cross-vendor proof: ZeroDDS <-> Cyclone DDS on the rich type, in all three
# XTypes extensibilities, both directions. Prints the received-sample count of
# every cell.
#
# Six cells: {@final, @appendable, @mutable} x {Cyclone -> ZeroDDS, ZeroDDS ->
# Cyclone}. Each cell gets a domain of its own, because a DDS domain is a UDP
# port band (RTPS 2.5 §9.6.1.1) and two cells on one domain read each other's
# traffic.
#
# Prereqs:
#   * the ZeroDDS side built:   (cd ../zerodds && cargo build --release)
#   * the Cyclone DDS Python binding: `python3 -c 'import cyclonedds'` works
#
# Env:
#   DOMAIN_BASE   first domain id   (default 73; six consecutive ids are used)
#   BLOB          sequence<octet> length (default 50)
#   PYTHON        the interpreter with the binding (default python3)
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
DOMAIN_BASE="${DOMAIN_BASE:-73}"
BLOB="${BLOB:-50}"
PY="${PYTHON:-python3}"
ZD="${CARGO_TARGET_DIR:-$HERE/../zerodds/target}/release"

for b in "$ZD/rich_pub" "$ZD/rich_sub"; do
    [ -x "$b" ] || { echo "missing $b — (cd ../zerodds && cargo build --release)"; exit 1; }
done
"$PY" -c 'import cyclonedds' 2>/dev/null || {
    echo "missing the Cyclone DDS Python binding ($PY -c 'import cyclonedds' fails)"; exit 1; }

# ---- the six domains this run will use have to be free ---------------------
command -v ss >/dev/null 2>&1 || {
    echo "ABORT: ss(8) is not installed, so this host cannot say whether the" \
         "domains are free; it will not measure into an unknown host"; exit 1; }
BUSY=""
for i in 0 1 2 3 4 5; do
    d=$((DOMAIN_BASE + i)); lo=$((7400 + 250 * d)); hi=$((lo + 249))
    hits="$(ss -uln 2>/dev/null | awk -v b="$lo" -v h="$hi" \
        '{n=$5; sub(/.*:/,"",n); if (n+0>=b && n+0<=h) print n}' | sort -un | tr '\n' ' ')"
    [ -n "$hits" ] && BUSY="$BUSY domain $d ($lo-$hi): $hits;"
done
if [ -n "$BUSY" ]; then
    echo "ABORT: the RTPS band of a domain this run needs is in use:$BUSY"
    ss -ulnp 2>/dev/null | head -40
    exit 1
fi
echo "domains $DOMAIN_BASE..$((DOMAIN_BASE + 5)) free"

LOG="$(mktemp -d)"

# ---- optional capture, on `any` --------------------------------------------
# Not on `lo`: SPDP/SEDP leave from ephemeral metatraffic ports outside the
# domain's band, and a capture narrowed to one interface or one port range has
# reported "no SEDP" for traffic that was on the wire.
CAP=""
if [ -n "${PCAP:-}" ] && command -v tcpdump >/dev/null 2>&1; then
    tcpdump -i any -s 0 -w "$PCAP" udp >"$LOG/tcpdump.log" 2>&1 &
    CAP=$!
    sleep 1
fi

# The processes this script started, by PID; `pkill -f <name>` matched every
# command line holding the name, the shell that started this script included.
# TERM first: `timeout` hands it on to its child, a KILL would orphan the child.
PIDS=""
cleanup() {
    for p in $PIDS; do kill "$p" 2>/dev/null; done
    sleep 1
    for p in $PIDS; do kill -9 "$p" 2>/dev/null; wait "$p" 2>/dev/null; done
    PIDS=""
}
finish() { cleanup; if [ -n "$CAP" ]; then kill "$CAP" 2>/dev/null; wait "$CAP" 2>/dev/null; fi; }
trap finish EXIT

FAILED=""
i=0
for ext in f a m; do
    dom=$((DOMAIN_BASE + i)); i=$((i + 1))
    echo "=== A/$ext) Cyclone pub -> ZeroDDS sub (domain $dom) ==="
    "$ZD/rich_sub" "$dom" "$ext" >"$LOG/${ext}_zs.log" 2>&1 &
    PIDS="$PIDS $!"
    sleep 2.5
    timeout 8 "$PY" "$HERE/cyc_rich_pub.py" "$dom" "$ext" "$BLOB" \
        >"$LOG/${ext}_cp.log" 2>&1
    sleep 1; cleanup; sleep 1
    n="$(grep -c 'integrity=OK' "$LOG/${ext}_zs.log")"
    bad="$(grep -c 'integrity=CORRUPT' "$LOG/${ext}_zs.log")"
    echo "ZeroDDS received from Cyclone (@$ext): $n  (corrupt: $bad)"
    grep 'integrity=' "$LOG/${ext}_zs.log" | head -1
    { [ "$n" -gt 0 ] && [ "$bad" = 0 ]; } || FAILED="$FAILED cyclone->zerodds/@$ext(n=$n,corrupt=$bad)"
    sleep 1
done

for ext in f a m; do
    dom=$((DOMAIN_BASE + i)); i=$((i + 1))
    echo "=== B/$ext) ZeroDDS pub -> Cyclone sub (domain $dom) ==="
    timeout 16 "$PY" "$HERE/cyc_rich_sub.py" "$dom" "$ext" \
        >"$LOG/${ext}_cs.log" 2>&1 &
    PIDS="$PIDS $!"
    sleep 3
    timeout 10 "$ZD/rich_pub" "$dom" "$ext" "$BLOB" \
        >"$LOG/${ext}_zp.log" 2>&1
    sleep 2; cleanup; sleep 1
    n="$(grep -c 'integrity=OK' "$LOG/${ext}_cs.log")"
    bad="$(grep -c 'integrity=CORRUPT' "$LOG/${ext}_cs.log")"
    echo "Cyclone received from ZeroDDS (@$ext): $n  (corrupt: $bad)"
    grep 'integrity=' "$LOG/${ext}_cs.log" | head -1
    { [ "$n" -gt 0 ] && [ "$bad" = 0 ]; } || FAILED="$FAILED zerodds->cyclone/@$ext(n=$n,corrupt=$bad)"
    sleep 1
done

echo
echo "logs: $LOG"
if [ -z "$FAILED" ]; then
    echo "RESULT: PASS (6 cells, both directions, all three extensibilities)"
else
    echo "RESULT: FAIL —$FAILED"; exit 1
fi
