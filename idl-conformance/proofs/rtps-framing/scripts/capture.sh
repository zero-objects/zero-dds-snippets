#!/usr/bin/env bash
# RTPS-framing vendor-oracle capture harness.
#
# Drives one DDS vendor (zerodds | cyclone | fastdds) to emit a full
# discovery + reliable-keyed-data exchange on domain 0, captures all RTPS
# traffic with tshark, and writes a pcapng + a field-level RTPS decode.
#
# Each run produces, under OUT/<vendor>/:
#   <vendor>.pcapng        raw capture (RTPS only, domain-0 ports)
#   <vendor>.rtps.txt      tshark -O rtps full protocol-tree decode
#   <vendor>.summary.txt   one line per submessage (frame, type, len)
#
# Usage:  capture.sh <vendor> <out_dir> [duration_secs]
set -u
VENDOR="${1:?vendor: zerodds|cyclone|fastdds}"
OUT="${2:?out dir}"
DUR="${3:-12}"
ZDDS=/tmp/oracle-rtps/zerodds
IFACE="${IFACE:-any}"
mkdir -p "$OUT/$VENDOR"
PCAP="$OUT/$VENDOR/$VENDOR.pcapng"

# RTPS magic 0x52 0x54 0x50 0x53 ("RTPS") at UDP payload start.
# Filter to domain-0 discovery+userdata ports to keep it tight, but match by
# RTPS magic so we never drop a stray port. (DG=250, d0=0 mcast, d1=10 ucast,
# d2=1 user mcast, d3=11 user ucast → 7400,7410,7401,7411 + ephemeral.)
FILTER='udp[8:4]=0x52545053'   # "RTPS"

echo "[capture] vendor=$VENDOR dur=${DUR}s -> $PCAP"
# Capture on lo (unicast SEDP/DATA/HEARTBEAT/ACKNACK, all on 127.0.0.1) AND
# eth0 (multicast SPDP 239.255.0.1). Two captures, merged at the end.
PCAP_LO="$OUT/$VENDOR/${VENDOR}.lo.pcapng"
PCAP_ETH="$OUT/$VENDOR/${VENDOR}.eth.pcapng"
tcpdump -i lo   -w "$PCAP_LO"  -U "$FILTER" >/dev/null 2>&1 &
TCPDUMP_LO=$!
tcpdump -i eth0 -w "$PCAP_ETH" -U "$FILTER" >/dev/null 2>&1 &
TCPDUMP_ETH=$!
TCPDUMP_PID="$TCPDUMP_LO $TCPDUMP_ETH"
sleep 1

case "$VENDOR" in
  zerodds)
    ( cd "$ZDDS" && ./target/debug/examples/shapes_demo_subscriber Square BlueShapes 0 >/tmp/oracle-rtps/z_sub.log 2>&1 ) &
    SUB=$!
    sleep 1
    ( cd "$ZDDS" && ./target/debug/examples/shapes_demo_publisher Square BlueShapes 0 BLUE >/tmp/oracle-rtps/z_pub.log 2>&1 ) &
    PUB=$!
    ;;
  cyclone)
    # ddsperf: keyed reliable ping/pong drives SPDP+SEDP+DATA+HEARTBEAT+ACKNACK.
    export CYCLONEDDS_URI='<CycloneDDS><Domain id="any"><General><Interfaces><NetworkInterface name="lo" multicast="true"/></Interfaces></General></Domain></CycloneDDS>'
    LD_LIBRARY_PATH=/opt/cyclone/lib /opt/cyclone/bin/ddsperf -D "$DUR" sub >/tmp/oracle-rtps/c_sub.log 2>&1 &
    SUB=$!
    sleep 1
    LD_LIBRARY_PATH=/opt/cyclone/lib /opt/cyclone/bin/ddsperf -D "$DUR" pub 2Hz >/tmp/oracle-rtps/c_pub.log 2>&1 &
    PUB=$!
    ;;
  fastdds)
    BIN=/tmp/oracle-rtps/fastdds_hello
    LD_LIBRARY_PATH=/opt/fastdds/lib "$BIN" sub >/tmp/oracle-rtps/f_sub.log 2>&1 &
    SUB=$!
    sleep 1
    LD_LIBRARY_PATH=/opt/fastdds/lib "$BIN" pub >/tmp/oracle-rtps/f_pub.log 2>&1 &
    PUB=$!
    ;;
  *) echo "unknown vendor"; kill $TCPDUMP_PID; exit 2;;
esac

sleep "$DUR"
kill "$PUB" "$SUB" 2>/dev/null
wait "$PUB" 2>/dev/null
sleep 1
kill -INT $TCPDUMP_PID 2>/dev/null
wait $TCPDUMP_PID 2>/dev/null

# Merge lo + eth0 captures into one ordered pcapng.
mergecap -w "$PCAP" "$PCAP_LO" "$PCAP_ETH" 2>/dev/null || cp "$PCAP_LO" "$PCAP"

echo "[decode] $OUT/$VENDOR/$VENDOR.rtps.txt"
tshark -r "$PCAP" -O rtps -V 2>/dev/null > "$OUT/$VENDOR/$VENDOR.rtps.txt"
tshark -r "$PCAP" -Y rtps 2>/dev/null \
  -T fields -e frame.number -e ip.src -e ip.dst -e rtps.sm.id -e udp.length \
  > "$OUT/$VENDOR/$VENDOR.summary.txt"
echo "[done] frames: $(tshark -r "$PCAP" -Y rtps 2>/dev/null | wc -l)"
