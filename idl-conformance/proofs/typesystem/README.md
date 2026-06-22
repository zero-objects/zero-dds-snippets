# XCDR2 type-system interop proof — unions, enums, `@optional`, nested `@mutable`

This example proves, **byte-for-byte against three independent DDS vendors**, how
ZeroDDS serialises the OMG IDL / DDS-XTypes 1.3 type-system constructs that are
easy to get subtly wrong on the wire:

* **unions** — discriminator + selected member, including a `default:` arm and a
  struct member arm, switching on both a `long` and an `enum`;
* **enums** — a plain 32-bit enum and (where the vendor supports it) `@bit_bound`
  8- and 16-bit enums;
* **standalone `@optional` members** — present and absent, of a primitive, a
  string, and a nested struct;
* **nested `@mutable`** — a `@mutable` struct as a member of another `@mutable`
  struct, and a `sequence<>` of a `@mutable` element (the EMHEADER framing).

Each construct is serialised by ZeroDDS to XCDR2 little-endian — the exact bytes
a ZeroDDS `DataWriter` puts on the wire — and compared against the **native**
encoding produced by CycloneDDS, RTI Connext and eProsima Fast DDS. The vendor
encodings are committed here as goldens so the comparison is reproducible without
the vendor toolchains installed.

> XCDR2 caps alignment at 4 bytes and frames `@appendable`/`@mutable` aggregates
> with a DHEADER / EMHEADER. The 4-byte CDR encapsulation header (representation
> id + options) is **stripped** from every golden so the files contain only the
> bare serialized payload.

## What each case proves

| case | sample | interop property |
|------|--------|------------------|
| `union_long_struct` | `LongUnion::Where{x=7,y=8}` (case 2) | long-switch union, **struct** member arm: discriminator then the nested struct, byte-identical to **all three** vendors |
| `union_enum_default` | `EnumUnion::Fallback(42)` (default, disc BLUE) | **enum-discriminated** union, **default** arm: byte-identical to Cyclone and RTI |
| `union_long_default` | `LongUnion::Note("hi")` (default) | long-switch union, **default** arm: byte-identical to Cyclone and RTI (FastDDS differs only in the synthesised default-discriminator value — see "Known divergences") |
| `opt_holder` | `{id=1, maybe_num=77, maybe_str=∅, maybe_obj={5,6}}` | standalone `@optional` (present primitive, **absent** string, present nested struct): byte-identical to Cyclone and RTI |
| `enum_holder` | `{big=BLUE, small=T_C, medium=MID_D}` | 32-bit + `@bit_bound(8)` + `@bit_bound(16)` enums: byte-identical to FastDDS (see "Known divergences" for the `@bit_bound` width question) |
| `mut_outer` | `{tag=9, leaf={100,1.25}, list=[{1,.5},{2,.25}]}` | nested `@mutable` + `sequence<@mutable>` EMHEADER framing: **byte-identical to Fast DDS** (LengthCode-5 reuse); round-trips. Cyclone/RTI differ only in which member gets LC5 — see "Known divergences" |

## Result table (this run)

ZeroDDS XCDR2-LE vs vendor-native, encapsulation stripped. `✓` = byte-identical.

| case | ZeroDDS | CycloneDDS | RTI Connext | Fast DDS |
|------|--------:|:----------:|:-----------:|:--------:|
| `union_long_struct`  | 12 B | ✓ | ✓ | ✓ |
| `union_enum_default` |  8 B | ✓ | ✓ | ✓ |
| `union_long_default` | 11 B | ✓ | ✓ | ≠ (default-disc value) |
| `opt_holder`         | 28 B | ✓ | ✓ | ≠ (uninitialised padding) |
| `enum_holder`        | 12 B | ≠ (`@bit_bound` width) | n/a (no `@bit_bound`) | ✓ |
| `mut_outer`          | 100 B | ≠ (Cyclone LC mix, 104 B) | ≠ (RTI LC mix, 100 B) | ✓ |

The `--check` harness prints this table from the committed goldens.

## Known divergences (read this — they are the point of the proof)

A vendor oracle is only honest if it reports the misses. Four of the six cases
diverge from at least one vendor; here is exactly why.

1. **`mut_outer` — nested `@mutable` (FIXED; byte-identical to Fast DDS).**
   ZeroDDS now encodes `mut_outer` to **100 B, byte-identical to Fast DDS**, and
   round-trips it through its own decoder (`nested_mutable_self_roundtrip`). Two
   coupled defects were fixed: (a) the writer-agnostic `CdrDecode` for a nested
   `@mutable` struct now parses the member EMHEADER frame (the `read_mutable_member`
   loop) instead of reading the EMHEADERs as plain sequential fields; (b) the
   encoder now uses the compact EMHEADER length-reuse form (**LengthCode 5**) for a
   member whose body begins with a length word — a nested `@appendable`/`@mutable`
   struct's DHEADER, a non-primitive `sequence`/`map` DHEADER, or a string length —
   instead of the redundant separate-NEXTINT form (LengthCode 4). A `@final` nested
   struct (no DHEADER) and a `sequence<primitive>` (bare element count) keep LC4.

   The vendors split on *which* member gets LC5 (this is implementation latitude,
   XTypes 1.3 §7.4.3.4.2): **Fast DDS** uses LC5 on both the nested struct (mid 20)
   and the sequence (mid 30) → 100 B; **Cyclone** uses LC5 only on the sequence and
   LC4 on the nested struct → 104 B; **RTI** uses LC5 on the nested struct and LC4
   on the sequence → 100 B. ZeroDDS matches Fast DDS's universal-LC5 form exactly.
   On DECODE, ZeroDDS reads back Cyclone's and Fast DDS's native bytes
   (`decode_vendor_mut_outer`); **RTI's** LC4 `sequence` member additionally OMITS
   the inner collection DHEADER from the member body (the NEXTINT is its sole
   delimiter — a framing RTI does not apply to its own LC5 nested-struct member),
   so ZeroDDS, which keeps the XCDR2-delimited sequence DHEADER like Fast DDS and
   Cyclone, does not blind-decode that specific non-self-delimited LC4 sequence.
   That is vendor framing latitude, not a ZeroDDS gap. The *other* five constructs
   are unaffected.

2. **`enum_holder` — `@bit_bound` enum width.** Per DDS-XTypes 1.3 §7.2.2.4.1.2 an
   `@bit_bound(8)` enum serialises as one octet and `@bit_bound(16)` as two.
   CycloneDDS honours this (8 B total); ZeroDDS and Fast DDS both serialise every
   enum as a 32-bit integer (12 B) and are byte-identical **to each other**. RTI
   Connext 7.7 rejects non-32-bit `@bit_bound` enums at code-generation time, so
   its golden carries only the 32-bit `Color`. The plain 32-bit enum
   (the `Color` discriminator in `union_enum_default`) matches every vendor.

3. **`union_long_default` — default-arm discriminator value.** When a union's
   default arm is selected, any discriminator value that is not an explicit case
   label is valid (§7.4.3). ZeroDDS, Cyclone and RTI serialise the natural value
   (0); Fast DDS synthesises `INT32_MAX` (`7f ff ff ff`). Only the 4 discriminator
   bytes differ; the selected member is identical, and either value routes to the
   default arm on decode.

4. **`opt_holder` — alignment padding.** The standalone `@optional` layout (a
   `uint8` presence flag followed by the aligned value) is byte-identical across
   ZeroDDS, Cyclone and RTI. Fast DDS differs only in two **padding** bytes inside
   an alignment gap, which FastCDR leaves uninitialised rather than zeroing.
   Padding values are unspecified by the standard; ZeroDDS zeroes them (so its
   output is deterministic and keyhash-stable).

## Vendor versions (this run, host "codepit", Debian 13, x86-64)

| vendor | code generator | runtime |
|--------|----------------|---------|
| CycloneDDS | `idlc` (Eclipse Cyclone DDS) **11.0.1** | `libddsc` 11.0.1, public `dds_cdrstream` API, XCDR2 |
| RTI Connext | `rtiddsgen` **4.7.0** (Connext **7.7.0**), C++11 | `to_cdr_buffer` with XCDR2 representation |
| eProsima Fast DDS | Fast DDS-Gen (**fastddsgen**) | Fast DDS **3.6.1** / FastCDR **2.3.5**, `Cdr` with `CdrVersion::XCDRv2` |
| ZeroDDS | `zerodds-idlc --rust` | `zerodds-cdr` core, XCDR2-LE |

## Files

```
typesystem/
├── README.md                 ← this file
├── idl/typesystem.idl        ← the corpus (one topic type per construct)
├── generated/typesystem.rs   ← ZeroDDS Rust, emitted by `zerodds-idlc --rust`
├── src/main.rs               ← the harness (encode + sha256 + --check byte-diff)
├── Cargo.toml
├── goldens/*.zerodds.bin     ← ZeroDDS XCDR2-LE encodings (regenerated by the run)
└── vendor/
    ├── cyclonedds/*.bin       ← native CycloneDDS XCDR2-LE (encapsulation stripped)
    ├── rti/*.bin              ← native RTI Connext XCDR2-LE
    └── fastdds/*.bin          ← native Fast DDS XCDR2-LE
```

ZeroDDS golden SHA-256 (this run):

```
enum_holder         79f2a4da15e31b933ef794b6f0a825d72d2bc94c887c03183baa43a1d09fdbff
union_long_default  0ba43f7ed32a68603f682b17df2e7a70077056e54f50c724e09b5f6a44c14c87
union_long_struct   9575df8994e60d8ce0930d937f8ec9503a8d231fcea1cc500c246fb0f6679839
union_enum_default  16aca604d9ac062a2c9a8ecdfc4d4b8e9cdf1b9c15e80de7100402dd56427700
opt_holder          ce26b98442c366fc6f0746668771b3ae2d353b08f665b17d794824f276847b10
mut_outer           d3a87bd9f80baa9d00f1c2b4cc9157adb9bb6cf4d9732c05fa7f937797b5bec7
```

## Reproduce

### ZeroDDS side (no vendors needed)

```sh
cd zerodds-examples/idl-conformance/proofs/typesystem

# Encode the canonical samples, print bytes + sha256, (re)write goldens/*.zerodds.bin
cargo run --bin typesystem_proof

# Byte-diff against the committed vendor goldens (prints the MATCH/DIFF table)
cargo run --bin typesystem_proof -- --check

# Self-roundtrip (incl. the now-fixed nested-@mutable) + decode-of-vendor-bytes
cargo test
```

Re-generate `generated/typesystem.rs` from the IDL with the ZeroDDS compiler:

```sh
cargo run -p zerodds-idlc --bin zerodds-idlc -- --rust -o generated idl/typesystem.idl
```

### Vendor side (to regenerate the goldens under `vendor/`)

On a host with the three SDKs. The bare XCDR2 payload is obtained by serialising
each canonical sample with the vendor's own type support and stripping the 4-byte
encapsulation header.

* **CycloneDDS** — `idlc typesystem.idl`, then a small C program using the public
  `dds_cdrstream` API: `dds_cdrstream_desc_from_topic_desc` + `dds_ostreamLE_init`
  (`DDSI_RTPS_CDR_ENC_VERSION_2`) + `dds_stream_write_sampleLE`; the ostream buffer
  is already encapsulation-free.
* **RTI Connext** — `rtiddsgen -language C++11 typesystem.idl`, then
  `rti::topic::to_cdr_buffer(buf, sample, dds::core::policy::DataRepresentation::xcdr2())`
  and drop the first 4 bytes.
* **Fast DDS** — `fastddsgen typesystem.idl`, then a program that constructs
  `eprosima::fastcdr::Cdr(buffer, DEFAULT_ENDIAN, CdrVersion::XCDRv2)` and does
  `cdr << sample` (no `serialize_encapsulation`), writing
  `get_serialized_data_length()` bytes.

The serialiser sources used for this run live under the scratch tree on the build
host; the resulting `*.bin` are committed here as the authoritative goldens.

> RTI rejects `@bit_bound` enums other than 32-bit, and the corpus avoids the
> enumerator names `M_E` / `M_PI` (they collide with `<math.h>` macros in the
> C/C++ vendor headers) by prefixing the 16-bit enum's members `MID_`.
