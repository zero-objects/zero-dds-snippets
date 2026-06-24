#!/usr/bin/env bash
# Bidirectional ZeroDDS <-> Cyclone DDS same-host ZERO-COPY over the iceoryx
# C++ (POSH) shared-memory transport. Both ends use the
# `zerodds-iceoryx-cyclone` crate (libiceoryx_binding_c FFI); Cyclone uses its
# installed psmx_iox plugin.
#
#   READ : a Cyclone publisher  -> the ZeroDDS reader bin  (expects READER OK)
#   WRITE: the ZeroDDS writer bin -> a Cyclone subscriber  (expects CYCLONE OK)
#
# Requirements (Linux only): the iceoryx POSH stack + iox-roudi + Cyclone DDS
# with the iox PSMX plugin (e.g. /opt/cyclone). Point CYCLONE at the Cyclone
# install and ZERODDS at a checked-out zero-dds source tree.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
: "${CYCLONE:=/opt/cyclone}"
: "${ZERODDS:=$HERE/../../..}"               # a zero-dds checkout (has crates/)
CRATE="$ZERODDS/crates/iceoryx-cyclone"
export LD_LIBRARY_PATH="$CYCLONE/lib:/usr/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH:-}"

# Two configs: ALLOW_NONDISCOVERED_WRITERS (so Cyclone accepts the ZeroDDS
# writer, which is not an RTPS-discovered DDS writer) also makes Cyclone's OWN
# publisher iox-ineligible, so READ uses the plain config and WRITE the
# allow-nondiscovered one.
READ_URI="file://$HERE/cyclone_read.xml"
WRITE_URI="file://$HERE/cyclone_write.xml"

cd "$HERE"
"$CYCLONE/bin/idlc" sample.idl
gcc cyclone_pub.c sample.c -I. -I"$CYCLONE/include" -L"$CYCLONE/lib" -lddsc -o cyclone_pub
gcc cyclone_sub.c sample.c -I. -I"$CYCLONE/include" -L"$CYCLONE/lib" -lddsc -o cyclone_sub
( cd "$CRATE" && cargo build --features posh )
READER="$CRATE/target/debug/zerodds-cyclone-iox-reader"
WRITER="$CRATE/target/debug/zerodds-cyclone-iox-writer"

pkill -f iox-roudi 2>/dev/null || true; sleep 1; rm -f /dev/shm/iceoryx* 2>/dev/null || true
iox-roudi -c roudi.toml >roudi.log 2>&1 &
ROUDI=$!
trap 'kill $ROUDI 2>/dev/null || true' EXIT
sleep 4

rc=0
echo "=== READ: Cyclone publisher -> ZeroDDS reader ==="
CYCLONEDDS_URI="$READ_URI" ./cyclone_pub >cyclone_pub.log 2>&1 &
PUB=$!
sleep 2
"$READER" | grep -q "READER OK" && echo "  READ PASS" || { echo "  READ FAIL"; rc=1; }
kill $PUB 2>/dev/null || true
sleep 1

echo "=== WRITE: ZeroDDS writer -> Cyclone subscriber ==="
CYCLONEDDS_URI="$WRITE_URI" ./cyclone_sub >cyclone_sub.log 2>&1 &
SUB=$!
sleep 2
"$WRITER" >/dev/null 2>&1 &
WR=$!
sleep 4
grep -q "CYCLONE OK" cyclone_sub.log && echo "  WRITE PASS" || { echo "  WRITE FAIL"; rc=1; }
kill $SUB $WR 2>/dev/null || true

[ $rc -eq 0 ] && echo "PROOF: bidirectional ZeroDDS <-> Cyclone iceoryx SHM — PASS" \
             || echo "PROOF: bidirectional ZeroDDS <-> Cyclone iceoryx SHM — FAIL"
exit $rc
