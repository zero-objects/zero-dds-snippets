#!/usr/bin/env bash
# Cross-vendor proof: ZeroDDS <-> eProsima Fast DDS on the rich type, in all
# three XTypes extensibilities, both directions. Prints the received-sample
# count of every cell.
#
# Six cells: {@final, @appendable, @mutable} x {Fast DDS -> ZeroDDS, ZeroDDS ->
# Fast DDS}. Each cell gets a domain of its own, because a DDS domain is a UDP
# port band (RTPS 2.5 §9.6.1.1) and two cells on one domain read each other's
# traffic.
#
# Prereqs:
#   * cargo: the ZeroDDS side is built here, in the Fast DDS dialect
#     (ZERODDS_RICH_DIALECT=fastdds, see ../zerodds/build.rs), into a target
#     directory of its own
#   * Fast DDS 3 installed under FASTDDS_HOME, fastddsgen and cmake. The peer
#     is generated from Rich.idl and built here the first time.
#
# Env:
#   FASTDDS_HOME  Fast DDS install  (default /opt/fastdds)
#   FASTDDSGEN    fastddsgen        (default: the one on PATH)
#   DOMAIN_BASE   first domain id   (default 73; six consecutive ids are used)
#   BLOB          sequence<octet> length (default 50)
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
# The Fast DDS reader announces no TypeConsistencyEnforcement (PID 0x0074).
# DDS-XTypes 1.3 §7.6.3.4.1 has a writer assume DISALLOW_TYPE_COERCION for it,
# which is the ZeroDDS default (xtypes.type_consistency_absent=disallow).
# The Rich types of the two sides are assignable but not equivalent.
# Under DISALLOW the ZeroDDS writer refuses the reader ("type.inconsistent")
# and the ZeroDDS -> Fast DDS direction receives nothing; this leg reads the
# absent policy as ALLOW_TYPE_COERCION.
export ZERODDS_TYPE_CONSISTENCY_ABSENT="${ZERODDS_TYPE_CONSISTENCY_ABSENT:-allow}"
DOMAIN_BASE="${DOMAIN_BASE:-73}"
BLOB="${BLOB:-50}"
FASTDDS_HOME="${FASTDDS_HOME:-/opt/fastdds}"
FASTDDSGEN="${FASTDDSGEN:-fastddsgen}"
BUILD="$HERE/build"
ZD_TARGET="$BUILD/zerodds-target"
ZD="$ZD_TARGET/release"

# ---- the ZeroDDS side, in the Fast DDS dialect -----------------------------
# Fast DDS writes a @mutable type as PL_CDR (XCDR1) with bare parameter ids
# and a bare 0x3f02 sentinel, and as PL_CDR2 (XCDR2) with the short length
# codes where they fit. A ZeroDDS codec built in the specification's dialect
# refuses the first as a framing error and writes the second in a form Fast
# DDS does not deliver: @mutable crosses in neither direction. The dialect is
# a build setting, so this leg has binaries of its own.
( cd "$HERE/../zerodds" &&
  CARGO_TARGET_DIR="$ZD_TARGET" ZERODDS_RICH_DIALECT=fastdds cargo build --quiet --release ) || {
    echo "cargo build of the ZeroDDS side (ZERODDS_RICH_DIALECT=fastdds) failed"; exit 1; }
for b in "$ZD/rich_pub" "$ZD/rich_sub"; do
    [ -x "$b" ] || { echo "missing $b after the build"; exit 1; }
done
command -v "$FASTDDSGEN" >/dev/null 2>&1 || { echo "missing fastddsgen — set FASTDDSGEN"; exit 1; }
[ -d "$FASTDDS_HOME/lib" ] || { echo "missing $FASTDDS_HOME/lib — set FASTDDS_HOME to a Fast DDS install"; exit 1; }
export LD_LIBRARY_PATH="$FASTDDS_HOME/lib:${LD_LIBRARY_PATH:-}"

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

# ---- the Fast DDS peer -----------------------------------------------------
if [ ! -x "$BUILD/bin/rich_fd_pub" ] || [ ! -x "$BUILD/bin/rich_fd_sub" ]; then
    echo "=== generating the Fast DDS type support and building the peer ==="
    mkdir -p "$BUILD/src"
    cp "$HERE/Rich.idl" "$HERE/pub.cxx" "$HERE/sub.cxx" "$HERE/CMakeLists.txt" "$BUILD/src/"
    ( cd "$BUILD/src" || exit 1
      "$FASTDDSGEN" -replace Rich.idl >gen.log 2>&1 || {
          echo "fastddsgen failed:"; tail -20 gen.log; exit 1; }
      cmake -S . -B ../cmake -DCMAKE_PREFIX_PATH="$FASTDDS_HOME" \
          -DCMAKE_BUILD_TYPE=Release >cmake.log 2>&1 &&
      cmake --build ../cmake -j >build.log 2>&1 || {
          echo "cmake failed:"; tail -30 cmake.log build.log; exit 1; }
      mkdir -p ../bin && cp ../cmake/rich_fd_pub ../cmake/rich_fd_sub ../bin/
    ) || exit 1
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
    echo "=== A/$ext) Fast DDS pub -> ZeroDDS sub (domain $dom) ==="
    "$ZD/rich_sub" "$dom" "$ext" >"$LOG/${ext}_zs.log" 2>&1 &
    PIDS="$PIDS $!"
    sleep 2.5
    timeout 8 "$BUILD/bin/rich_fd_pub" "$dom" "$ext" "$BLOB" \
        >"$LOG/${ext}_fp.log" 2>&1
    sleep 1; cleanup; sleep 1
    n="$(grep -c 'integrity=OK' "$LOG/${ext}_zs.log")"
    bad="$(grep -c 'integrity=CORRUPT' "$LOG/${ext}_zs.log")"
    echo "ZeroDDS received from Fast DDS (@$ext): $n  (corrupt: $bad)"
    grep 'integrity=' "$LOG/${ext}_zs.log" | head -1
    { [ "$n" -gt 0 ] && [ "$bad" = 0 ]; } || FAILED="$FAILED fastdds->zerodds/@$ext(n=$n,corrupt=$bad)"
    sleep 1
done

for ext in f a m; do
    dom=$((DOMAIN_BASE + i)); i=$((i + 1))
    echo "=== B/$ext) ZeroDDS pub -> Fast DDS sub (domain $dom) ==="
    timeout 16 "$BUILD/bin/rich_fd_sub" "$dom" "$ext" \
        >"$LOG/${ext}_fs.log" 2>&1 &
    PIDS="$PIDS $!"
    sleep 3
    timeout 10 "$ZD/rich_pub" "$dom" "$ext" "$BLOB" \
        >"$LOG/${ext}_zp.log" 2>&1
    sleep 2; cleanup; sleep 1
    n="$(grep -c 'integrity=OK' "$LOG/${ext}_fs.log")"
    bad="$(grep -c 'integrity=CORRUPT' "$LOG/${ext}_fs.log")"
    echo "Fast DDS received from ZeroDDS (@$ext): $n  (corrupt: $bad)"
    grep 'integrity=' "$LOG/${ext}_fs.log" | head -1
    { [ "$n" -gt 0 ] && [ "$bad" = 0 ]; } || FAILED="$FAILED zerodds->fastdds/@$ext(n=$n,corrupt=$bad)"
    sleep 1
done

echo
echo "logs: $LOG"
if [ -z "$FAILED" ]; then
    echo "RESULT: PASS (6 cells, both directions, all three extensibilities)"
else
    echo "RESULT: FAIL —$FAILED"; exit 1
fi
