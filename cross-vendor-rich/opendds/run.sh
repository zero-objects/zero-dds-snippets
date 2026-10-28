#!/usr/bin/env bash
# Cross-vendor proof: ZeroDDS <-> OpenDDS on the rich type, in all three
# XTypes extensibilities, both directions. Prints the received-sample count of
# every cell.
#
# Six cells: {@final, @appendable, @mutable} x {OpenDDS -> ZeroDDS, ZeroDDS ->
# OpenDDS}. Each cell gets a domain of its own, because a DDS domain is a UDP
# port band (RTPS 2.5 §9.6.1.1) and two cells on one domain read each other's
# traffic.
#
# Prereqs:
#   * cargo: the ZeroDDS side is built here, in the dialect ZD_DIALECT names
#     (see ../zerodds/build.rs), into a target directory of its own
#   * an OpenDDS source tree with RTPS, built, under DDS_ROOT (its setenv.sh
#     puts opendds_idl, ACE/TAO and the libraries on the paths), and cmake.
#     The peer (pub.cpp, sub.cpp, Rich.idl here) is built the first time.
#
# Env:
#   DDS_ROOT      the OpenDDS tree  (required)
#   ZD_DIALECT    CDR dialect of the ZeroDDS side (default specification)
#   DOMAIN_BASE   first domain id   (default 73; six consecutive ids are used)
#   BLOB          sequence<octet> length (default 50)
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
DOMAIN_BASE="${DOMAIN_BASE:-73}"
BLOB="${BLOB:-50}"
ZD_DIALECT="${ZD_DIALECT:-specification}"
BUILD="$HERE/build"
ZD_TARGET="$BUILD/zerodds-target-$ZD_DIALECT"
ZD="$ZD_TARGET/release"

# ---- the ZeroDDS side, in ZD_DIALECT ---------------------------------------
( cd "$HERE/../zerodds" &&
  CARGO_TARGET_DIR="$ZD_TARGET" ZERODDS_RICH_DIALECT="$ZD_DIALECT" cargo build --quiet --release ) || {
    echo "cargo build of the ZeroDDS side (ZERODDS_RICH_DIALECT=$ZD_DIALECT) failed"; exit 1; }
for b in "$ZD/rich_pub" "$ZD/rich_sub"; do
    [ -x "$b" ] || { echo "missing $b after the build"; exit 1; }
done
[ -f "${DDS_ROOT:-}/setenv.sh" ] || { echo "missing \$DDS_ROOT/setenv.sh — set DDS_ROOT to a built OpenDDS tree"; exit 1; }
# setenv.sh appends to LD_LIBRARY_PATH and PATH as it finds them, which
# `set -u` refuses when the first is unset.
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
# shellcheck disable=SC1091
. "$DDS_ROOT/setenv.sh"

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

# ---- the OpenDDS peer ------------------------------------------------------
if [ ! -x "$BUILD/cmake/od_pub" ] || [ ! -x "$BUILD/cmake/od_sub" ]; then
    echo "=== generating the OpenDDS type support and building the peer ==="
    mkdir -p "$BUILD"
    cmake -S "$HERE" -B "$BUILD/cmake" -DOpenDDS_ROOT="$DDS_ROOT" \
        -DCMAKE_BUILD_TYPE=Release >"$BUILD/cmake.log" 2>&1 &&
    cmake --build "$BUILD/cmake" -j >"$BUILD/build.log" 2>&1 || {
        echo "cmake failed:"; tail -30 "$BUILD/cmake.log" "$BUILD/build.log"; exit 1; }
fi

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
    echo "=== A/$ext) OpenDDS pub -> ZeroDDS sub (domain $dom) ==="
    "$ZD/rich_sub" "$dom" "$ext" >"$LOG/${ext}_zs.log" 2>&1 &
    PIDS="$PIDS $!"
    sleep 2.5
    ( cd "$HERE" && timeout 8 "$BUILD/cmake/od_pub" -DCPSConfigFile rtps.ini "$dom" "$ext" "$BLOB" ) \
        >"$LOG/${ext}_op.log" 2>&1
    sleep 1; cleanup; sleep 1
    n="$(grep -c 'integrity=OK' "$LOG/${ext}_zs.log")"
    bad="$(grep -c 'integrity=CORRUPT' "$LOG/${ext}_zs.log")"
    echo "ZeroDDS received from OpenDDS (@$ext): $n  (corrupt: $bad)"
    grep 'integrity=' "$LOG/${ext}_zs.log" | head -1
    { [ "$n" -gt 0 ] && [ "$bad" = 0 ]; } || FAILED="$FAILED opendds->zerodds/@$ext(n=$n,corrupt=$bad)"
    sleep 1
done

for ext in f a m; do
    dom=$((DOMAIN_BASE + i)); i=$((i + 1))
    echo "=== B/$ext) ZeroDDS pub -> OpenDDS sub (domain $dom) ==="
    ( cd "$HERE" && exec timeout 16 "$BUILD/cmake/od_sub" -DCPSConfigFile rtps.ini "$dom" "$ext" ) \
        >"$LOG/${ext}_os.log" 2>&1 &
    PIDS="$PIDS $!"
    sleep 3
    # An OpenDDS 3.34 DataReader accepts XCDR2 only and drops an XCDR1
    # sample ("Encoding kind CDR/XCDR1 of the received sample does not
    # match"); ZeroDDS's writer offers XCDR1 first unless told otherwise
    # (docs/doc-map/interop-and-dialects.md, "OpenDDS 3.34.0").
    ZERODDS_DATA_REPR_OFFER=XCDR2 timeout 10 "$ZD/rich_pub" "$dom" "$ext" "$BLOB" \
        >"$LOG/${ext}_zp.log" 2>&1
    sleep 2; cleanup; sleep 1
    n="$(grep -c 'integrity=OK' "$LOG/${ext}_os.log")"
    bad="$(grep -c 'integrity=CORRUPT' "$LOG/${ext}_os.log")"
    echo "OpenDDS received from ZeroDDS (@$ext): $n  (corrupt: $bad)"
    grep 'integrity=' "$LOG/${ext}_os.log" | head -1
    { [ "$n" -gt 0 ] && [ "$bad" = 0 ]; } || FAILED="$FAILED zerodds->opendds/@$ext(n=$n,corrupt=$bad)"
    sleep 1
done

echo
echo "logs: $LOG"
if [ -z "$FAILED" ]; then
    echo "RESULT: PASS (6 cells, both directions, all three extensibilities)"
else
    echo "RESULT: FAIL —$FAILED"; exit 1
fi
