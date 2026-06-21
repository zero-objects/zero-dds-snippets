# idl-conformance — Rust

A **standalone, runnable** real-DCPS conformance roundtrip for the ZeroDDS Rust
language binding (PSM).

It takes a richly-combined IDL type, generates Rust types with
`zerodds-idlc --rust`, then over a **real `DomainParticipant` + pub/sub UDP
loopback** (two participants in one process, same domain) publishes one
fully-populated sample and reads it back, asserting field-by-field that the data
survived the XCDR2 wire. This is an end-to-end DCPS roundtrip, **not** an
in-memory encode/decode check.

## IDL features exercised

The topic type is `combo::Telemetry`, derived from the upstream conformance
fixture `tools/idlc/tests/conformance/fixtures/20_mixed_combo.idl`. One sample
combines, in a single keyed topic type:

| Feature                       | Member                                     |
|-------------------------------|--------------------------------------------|
| enum                          | `Mode mode`                                |
| typedef-to-primitive          | `CurrentInAmpsType batteryCurrent` (=double) |
| nested struct                 | `Sample` (inside the sequence)             |
| sequence-of-struct            | `sequence<Sample> history`                 |
| bounded string                | `string<32> region`, `string<16>` union arm |
| union over an enum discriminator | `Reading reading` (`switch(Mode)`)      |
| map                           | `map<string,long> counters`                |
| @optional                     | `@optional double calibration`             |
| fixed-size array              | `long window[4]`                           |
| @key (multi-field)            | `@key long unitId`, `@key string<32> region` |
| @appendable extensibility     | the `Telemetry` struct itself              |

All of the above round-trip successfully over real DCPS. The topic type is the
**full** upstream combo fixture — every member is present, none excluded.

## Build & run

From this directory:

```sh
# 1. (Optional) regenerate the Rust types from the IDL. The committed
#    generated/20_mixed_combo.rs was produced exactly this way.
../../../target/debug/zerodds-idlc generate \
    idl/20_mixed_combo.idl --rust -o generated </dev/null

# 2. Build and run the roundtrip.
cargo run --bin combo_roundtrip
```

Expected output (last lines):

```
PUB  -> Telemetry { unitId: 4242, region: "eu-central-1", mode: MODE_ACTIVE, ... }
SUB  <- Telemetry { unitId: 4242, region: "eu-central-1", mode: MODE_ACTIVE, ... }

OK: DCPS pub->sub roundtrip recovered the sample; all 9 field groups matched.
```

The program `assert_eq!`s each field group and then the whole struct, so a
non-zero exit means a roundtrip mismatch.

Path deps reach into the local zerodds checkout (`../../../crates/{dcps, cdr,
types, sql-filter}`) — `cdr`, `types`, and `sql-filter` are pulled in because
the generated `DdsType` impls reference `zerodds_cdr::BufferWriter`,
`zerodds_types::TypeIdentifier`, and `zerodds_sql_filter::Value`.

## Known limitations

* `zerodds-idlc` prints `TypeObject emission skipped — UnsupportedTypeSpec("map
  (inline IDL map)")` while generating. This only skips the optional
  XTypes `TypeObject`/`TypeIdentifier` metadata for the map member (the struct's
  `TYPE_IDENTIFIER` becomes `None`); the runtime encode/decode for the map is
  generated correctly and the map round-trips, as the assertions confirm.

* The generated enum variants keep the IDL spelling (`MODE_ACTIVE`, …), so the
  build emits three `non_camel_case_types` *warnings* (cosmetic only — the code
  compiles, runs, and round-trips). This is a lint style choice in the codegen,
  not a correctness gap.

The fixed-size array-member decode bug is **resolved**: the full upstream combo
(`long window[4]` included) now generates correct decode code
(`<[i32; 4] as CdrDecode>::decode(r)`) and the array round-trips. There are no
excluded members — this example exercises the complete `20_mixed_combo.idl`.

## Reference

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS
implementation. Companion to the **[Rust bindings](https://zerodds.de/bindings/rust/)**
page.
