#!/usr/bin/env bash
# Cross-vendor rich-type interop: ZeroDDS <-> OpenSplice Community Edition 6.9,
# the @final cell (RichF) in both directions. OpenSplice has no extensibility
# kinds, so there is no @appendable or @mutable cell against it.
#
# Prereqs: OpenSplice 6.9 under OSPL_HOME (release.com, idlpp, the classic C++
# API) and g++; the ZeroDDS side is built here with cargo.
#
# Env:
#   OSPL_HOME     the OpenSplice install  (default /opt/opensplice-6.9)
#   DOMAIN_BASE   first domain id         (default 73; two consecutive ids)
#   BLOB          sequence<octet> length  (default 50)
#
# OpenSplice takes its domain from its configuration file: run.sh writes one
# per domain and points OSPL_URI at it.
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
DOMAIN_BASE="${DOMAIN_BASE:-73}"
BLOB="${BLOB:-50}"
OSPL_HOME="${OSPL_HOME:-/opt/opensplice-6.9}"
BUILD="$HERE/build"
ZD_TARGET="$BUILD/zerodds-target"
ZD="$ZD_TARGET/release"

# ---- the ZeroDDS side ------------------------------------------------------
( cd "$HERE/../zerodds" && CARGO_TARGET_DIR="$ZD_TARGET" cargo build --quiet --release ) || {
    echo "cargo build of the ZeroDDS side failed"; exit 1; }
for b in "$ZD/rich_pub" "$ZD/rich_sub"; do
    [ -x "$b" ] || { echo "missing $b after the build"; exit 1; }
done
[ -x "$OSPL_HOME/bin/idlpp" ] || { echo "missing $OSPL_HOME/bin/idlpp — set OSPL_HOME to an OpenSplice 6.9 install"; exit 1; }
[ $((DOMAIN_BASE + 1)) -le 230 ] || { echo "OpenSplice 6.9 refuses domain ids above 230"; exit 1; }
# release.com reads LD_LIBRARY_PATH unset, which `set -u` refuses.
set +u
# shellcheck disable=SC1091
. "$OSPL_HOME/release.com" >/dev/null
set -u

# ---- the two domains this run will use have to be free ---------------------
command -v ss >/dev/null 2>&1 || {
    echo "ABORT: ss(8) is not installed, so this host cannot say whether the" \
         "domains are free; it will not measure into an unknown host"; exit 1; }
BUSY=""
for i in 0 1; do
    d=$((DOMAIN_BASE + i)); lo=$((7400 + 250 * d)); hi=$((lo + 249))
    hits="$(ss -uln 2>/dev/null | awk -v b="$lo" -v h="$hi" \
        '{n=$5; sub(/.*:/,"",n); if (n+0>=b && n+0<=h) print n}' | sort -un | tr '\n' ' ')"
    [ -n "$hits" ] && BUSY="$BUSY domain $d ($lo-$hi): $hits;"
done
if [ -n "$BUSY" ]; then
    echo "ABORT: the RTPS band of a domain this run needs is in use:$BUSY"
    exit 1
fi
echo "domains $DOMAIN_BASE..$((DOMAIN_BASE + 1)) free"

# ---- the OpenSplice peer ---------------------------------------------------
if [ ! -x "$BUILD/ospl_pub" ] || [ ! -x "$BUILD/ospl_sub" ]; then
    echo "=== generating the OpenSplice type support and building the peer ==="
    mkdir -p "$BUILD"
    cp "$HERE/Rich.idl" "$HERE/pub.cpp" "$HERE/sub.cpp" "$BUILD/"
    (
        cd "$BUILD" &&
        idlpp -S -l cpp Rich.idl > idlpp.log 2>&1 &&
        for side in pub sub; do
            g++ -std=c++11 -O1 -I. -I"$OSPL_HOME/include" -I"$OSPL_HOME/include/sys" \
                -I"$OSPL_HOME/include/dcps/C++/SACPP" -o "ospl_$side" "$side.cpp" \
                Rich.cpp RichDcps.cpp RichDcps_impl.cpp RichSplDcps.cpp \
                -L"$OSPL_HOME/lib" -ldcpssacpp -lddskernel -Wl,-rpath,"$OSPL_HOME/lib" || exit 1
        done
    ) > "$BUILD/build.log" 2>&1 || { echo "build failed:"; tail -30 "$BUILD/idlpp.log" "$BUILD/build.log"; exit 1; }
fi
config_for() { # <domain> -> the configuration file for it
    sed "s#<Id>0</Id>#<Id>$1</Id>#" "$OSPL_HOME/etc/config/ospl_sp_ddsi.xml" > "$BUILD/ospl-$1.xml"
    echo "file://$BUILD/ospl-$1.xml"
}

LOG="$(mktemp -d)"
PIDS=""
cleanup() {
    for p in $PIDS; do kill "$p" 2>/dev/null; done
    sleep 1
    for p in $PIDS; do kill -9 "$p" 2>/dev/null; wait "$p" 2>/dev/null; done
    PIDS=""
}
trap cleanup EXIT

FAILED=""
dom=$DOMAIN_BASE
echo "=== A/f) OpenSplice pub -> ZeroDDS sub (domain $dom) ==="
"$ZD/rich_sub" "$dom" f >"$LOG/f_zs.log" 2>&1 &
PIDS="$PIDS $!"
sleep 2.5
( cd "$LOG" && OSPL_URI="$(config_for "$dom")" timeout 8 "$BUILD/ospl_pub" "$BLOB" ) >"$LOG/f_op.log" 2>&1
sleep 1; cleanup; sleep 1
n="$(grep -c 'integrity=OK' "$LOG/f_zs.log")"
bad="$(grep -c 'integrity=CORRUPT' "$LOG/f_zs.log")"
echo "ZeroDDS received from OpenSplice (@f): $n  (corrupt: $bad)"
grep 'integrity=' "$LOG/f_zs.log" | head -1
{ [ "$n" -gt 0 ] && [ "$bad" = 0 ]; } || FAILED="$FAILED opensplice->zerodds/@f(n=$n,corrupt=$bad)"
sleep 1

dom=$((DOMAIN_BASE + 1))
echo "=== B/f) ZeroDDS pub -> OpenSplice sub (domain $dom) ==="
( cd "$LOG" && OSPL_URI="$(config_for "$dom")" exec timeout 16 "$BUILD/ospl_sub" ) >"$LOG/f_os.log" 2>&1 &
PIDS="$PIDS $!"
sleep 3
timeout 10 "$ZD/rich_pub" "$dom" f "$BLOB" >"$LOG/f_zp.log" 2>&1
sleep 2; cleanup; sleep 1
n="$(grep -c 'integrity=OK' "$LOG/f_os.log")"
bad="$(grep -c 'integrity=CORRUPT' "$LOG/f_os.log")"
echo "OpenSplice received from ZeroDDS (@f): $n  (corrupt: $bad)"
grep 'integrity=' "$LOG/f_os.log" | head -1
{ [ "$n" -gt 0 ] && [ "$bad" = 0 ]; } || FAILED="$FAILED zerodds->opensplice/@f(n=$n,corrupt=$bad)"

echo
echo "logs: $LOG"
if [ -z "$FAILED" ]; then
    echo "RESULT: PASS (the @final cell, both directions)"
else
    echo "RESULT: FAIL —$FAILED"; exit 1
fi
