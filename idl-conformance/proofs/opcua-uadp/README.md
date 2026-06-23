# OPC-UA UADP wire proof — byte-identical to open62541

This proves that ZeroDDS (`zerodds-opcua-pubsub`) serializes an OPC-UA PubSub
**UADP `NetworkMessage`** (OPC Foundation Part 14 §7.2.2) **byte-for-byte
identically** to the OPC Foundation reference implementation **open62541 1.5.4**
(PubSub enabled).

Until now the UADP codec was only checked for *internal* self-consistency
(encode → decode round-trips). Internal consistency does not prove conformance
to the spec — a whole-codec offset or flag-bit mistake round-trips cleanly while
diverging from every other implementation. This proof closes that gap by
diffing ZeroDDS's bytes against an independent encoder.

## Result

All five vectors are **byte-identical** (ZeroDDS == open62541), first run, no
fixes needed:

| vector | configuration | len | result |
|--------|---------------|-----|--------|
| V1 | `UInt16` PublisherId, WriterGroup (id 7, seq 99), 1 KeyFrame, two `UInt32` Variant fields | 25 | **MATCH** |
| V2 | `Byte` PublisherId, 1 KeyFrame with `Boolean` + `Double` + `Int16` Variant fields | 27 | **MATCH** |
| V3 | `UInt16` PublisherId, 1 KeyFrame with a `String` Variant field | 22 | **MATCH** |
| V4 | two DataSetMessages (PayloadHeader writer-id list + payload **size array**) | 34 | **MATCH** |
| V5 | DataSetMessage carrying its own SequenceNumber (the **DataSetFlags2** path) | 22 | **MATCH** |

The vectors deliberately exercise the framing decisions most likely to drift
between implementations: the UADP flag bytes + ExtendedFlags1, every PublisherId
type encoding, the GroupHeader flag set, the PayloadHeader writer-id list, the
multi-message payload size array, the DataSetMessage DataSetFlags1 field
encoding, the DataSetFlags2 sequence-number path, and Variant scalar encodings
across integer / floating / boolean / string built-in types.

## The two sides

* **open62541** (`vendor-src/uadp_oracle.c`) builds a `UA_NetworkMessage` for
  each vector and calls `UA_NetworkMessage_encodeBinary`, dumping the bytes.
  The captured output is `goldens.txt`.
* **ZeroDDS** builds the *same logical message* with `zerodds-opcua-pubsub`'s
  `NetworkMessage` / `DataSetMessage` API and `to_binary`. The conformance gate
  `crates/opcua-pubsub/src/uadp/network_message.rs::uadp_byte_identical_to_open62541`
  asserts each vector equals the `goldens.txt` hex, so a future regression in
  either the framing or a built-in encoder fails the build.

## Reproduce

ZeroDDS side:

```sh
cargo test -p zerodds-opcua-pubsub --lib uadp_byte_identical_to_open62541
```

Vendor side (on a host with open62541 1.5 built with `UA_ENABLE_PUBSUB`):

```sh
cd vendor-src
gcc uadp_oracle.c -I<o62541>/include -I<o62541>/build/src_generated \
    -L<o62541>/build/bin -lopen62541 -o uadp_oracle
./uadp_oracle          # prints "OPEN62541_UADP <vector> <len> <hex>" per vector
```

See `vendor-src/VENDOR-VERSIONS.txt` for the exact toolchain.
