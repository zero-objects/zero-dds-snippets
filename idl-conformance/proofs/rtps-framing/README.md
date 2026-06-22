# RTPS-Framing Conformance Proof — ZeroDDS vs Cyclone DDS vs Fast-DDS

This proof byte-diffs the **DDSI-RTPS 2.5 submessage envelope** that ZeroDDS
puts on the wire against two reference implementations, Eclipse Cyclone DDS and
eProsima Fast-DDS. Earlier conformance work checked the serialized DATA *payload*
(CDR / XCDR2). This project checks the *framing* around it: the submessage
headers, the SPDP / SEDP discovery `ParameterList`s, and the reliable-protocol
submessages (HEARTBEAT / ACKNACK / GAP / DATA_FRAG / INFO_*).

All three stacks were driven to emit a full discovery + reliable keyed-data
exchange on **domain 0, loopback**, captured with `tshark`, and decoded with
Wireshark's mature RTPS dissector at the field level.

## TL;DR result

Wireshark's RTPS dissector reports **zero `Malformed` and zero Expert-Info
markers** across every ZeroDDS submessage. Every `octetsToNextHeader`, every
`ParameterList` length/alignment, the `PID_SENTINEL` terminator, and the
endianness flags are correct and decode cleanly.

* **SubmessageHeader** (id / flags / octetsToNextHeader): MATCH on all types.
* **SEDP** publication `ParameterList`: **byte-identical PID set and order**
  across all three vendors (14 PIDs).
* **SPDP** participant `ParameterList`: ZeroDDS == Fast-DDS exactly; Cyclone
  adds optional + vendor-specific PIDs (legitimate latitude, §9.6.2).
* **HEARTBEAT / ACKNACK / INFO_DST / DATA / DATA_FRAG**: framing MATCH.
* One genuine gap: **ZeroDDS emits no `INFO_TS` (0x09)** — the writer never
  attaches a source timestamp. Both Cyclone and Fast-DDS prefix their DATA with
  INFO_TS. This is missing functionality (source-timestamp / `BY_SOURCE_TIMESTAMP`
  destination ordering), not a malformed frame. See `analysis/NOTES.txt` and the
  findings section below.

## Environment

| Component   | Version |
|-------------|---------|
| ZeroDDS     | `main` @ commit recorded in `analysis/NOTES.txt` (debug build) |
| Cyclone DDS | 11.0.1 (`ddsperf`) |
| Fast-DDS    | 3.6.1 (custom keyed HelloWorld, `scripts/fastdds_hello.cpp`) |
| tshark      | Wireshark 4.4.15 |
| OS          | Debian 13 (trixie), Linux 6.17 |

## Layout

```
captures/          merged pcapng per vendor (steady-state discovery + keyed data)
frag/              DATA_FRAG captures (64 KiB sample) + decodes, ZeroDDS + Cyclone
decodes/           tshark -O rtps -V field-tree of one representative of each
                   submessage type, per vendor
hex/               raw byte dump (tshark -x) of the same representative frames
analysis/          submessage_counts.txt, spdp_pids.txt, sedp_pub_pids.txt, NOTES.txt
scripts/           capture.sh (the harness) + fastdds_hello.cpp (the FastDDS driver)
```

## Reproduce

On a Linux host with the three stacks installed, `tshark`/`tcpdump`, and the
ZeroDDS workspace built:

```bash
# Build the ZeroDDS traffic drivers (real UDP + multicast discovery)
cargo build -p zerodds-dcps \
  --example shapes_demo_publisher --example shapes_demo_subscriber \
  --example largedata_pub --example largedata_sub

# Build the Fast-DDS driver (keyed HelloWorld)
g++ -std=c++17 scripts/fastdds_hello.cpp -o fastdds_hello \
  -I/opt/fastdds/include -L/opt/fastdds/lib -lfastdds -lfastcdr \
  -Wl,-rpath,/opt/fastdds/lib

# Capture each vendor's full exchange (domain 0). capture.sh runs pub+sub,
# records lo (unicast SEDP/DATA/HEARTBEAT) + eth0 (multicast SPDP), merges,
# and emits the -O rtps decode + a per-submessage summary.
ZDDS=/path/to/zerodds bash scripts/capture.sh zerodds ./out 12
                       bash scripts/capture.sh cyclone ./out 12   # uses ddsperf
                       bash scripts/capture.sh fastdds ./out 12   # uses ./fastdds_hello
```

> Note on addresses: the capture host's private LAN address (RFC 1918,
> `192.168.x.x`) appears in the recorded SPDP locators and as the multicast
> source IP — this is normal DDS wire content. Text decodes use the RFC 5737
> documentation address `203.0.113.10` for readability.

The exact decode/extraction commands (per submessage, by resolving each
representative `frame.number`) are documented inline in `analysis/NOTES.txt`.
A handy one-liner for the submessage distribution:

```bash
tshark -r captures/zerodds.pcapng -Y rtps -T fields -e rtps.sm.id \
  | tr ',\t' '\n\n' | sort | uniq -c
```

## What byte-matches vs what is legitimate latitude

DDSI-RTPS 2.5 §8.3.4 fixes the SubmessageHeader layout exactly, and §9.4.2.11
fixes the `ParameterList` TLV format (4-byte-aligned, sentinel-terminated). Those
are hard byte constraints — ZeroDDS matches them. What the spec does **not** fix:

* **PID ordering and the optional-PID set in SPDP** (§9.6.2). A receiver must
  accept PIDs in any order and ignore unknown optional PIDs. Cyclone shipping
  `PID_USER_DATA`, `PID_PROPERTY_LIST`, `PID_DEFAULT_MULTICAST_LOCATOR`, and its
  vendor PIDs `0x8007`/`0x8019` is fully conformant; ZeroDDS omitting the empty
  ones is too.
* **INFO_TS attachment** is optional per submessage, but a writer that never
  sends it leaves every received sample with an invalid source timestamp. This
  is the one substantive divergence (a functional gap, captured as a finding).
* **Fragment size.** ZeroDDS uses a path-MTU-aware fragment size: 63000 B when
  all peers are on loopback (MTU 65536), 1344 B off-host — identical to Cyclone's
  1344 B network fragment. The large loopback fragment is an optimization, not a
  defect; on a real Ethernet path ZeroDDS fragments at the MTU-safe 1344 B.
* **Inline QoS (the Q flag) on steady-state user DATA** is optional. Neither
  ZeroDDS nor Fast-DDS sets it for plain keyed writes; Cyclone sets it on a
  subset. KeyHash / StatusInfo inline QoS is required only for the dispose /
  unregister / content-filter lifecycle, not for every sample.

See `analysis/NOTES.txt` for the per-vendor numbers behind each statement.
