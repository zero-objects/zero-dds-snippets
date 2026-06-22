# TypeObject / TypeIdentifier byte-identity proof (DDS-XTypes 1.3 §7.3.4)

This proof project byte-diffs **ZeroDDS's serialized `TypeObject` and the
14-byte `TypeIdentifier` EquivalenceHash** against three independent
XTypes-1.3 implementations — **CycloneDDS, RTI Connext, eProsima Fast DDS** —
for a documented corpus of canonical types.

Why this matters: remote type discovery and type assignability (the XTypes
TypeLookup service + `PID_TYPE_INFORMATION` in SEDP) correlate types across
vendors **purely by the 14-byte hash of the serialized TypeObject**. If two
implementations serialize "the same" type to different bytes, they derive
different hashes, and a remote reader/writer cannot recognize the peer's type.
Behavioural matching of typed endpoints (proven separately) is **not**
sufficient — the serialized TypeObject and its hash must be byte-identical.

## The spec rule (§7.3.4.5 / §7.3.1.2.1)

A `TypeObject` is itself serialized, with these mandatory constraints so the
result is bitwise identical across vendors:

* **XCDR encoding version 2, Little-Endian.**
* Aggregated members are ordered by `member_index` (struct/union), enum
  literals by numeric value, annotations by hash.

The `TypeIdentifier` for a complex type is then
`{ EquivalenceKind, EquivalenceHash }` where:

```
EquivalenceHash(T) = MD5( serialized_TypeObject(T) )[0 .. 14]
```

`EquivalenceKind` is `EK_MINIMAL = 0xF1` or `EK_COMPLETE = 0xF2`.

**The exact MD5 input** (verified empirically — see below): the XCDR2 stream of
the TypeObject, i.e. a 4-byte XCDR2 **DHEADER** (the byte length of the body)
followed by the body `[ EquivalenceKind octet | TypeKind octet | … ]`. For the
`Plain` minimal type all three vendors hash the **same 55 bytes** and obtain
the **same hash**.

## The canonical corpus (`idl/typeobject.idl`)

| case | construct |
|------|-----------|
| `plain` | `@final` struct (`long`, `short`) |
| `appendable` | `@appendable` struct |
| `mutable` | `@mutable` struct with explicit `@id` members |
| `color_enum` | enum (`RED`/`GREEN`/`BLUE`) |
| `long_seq4_alias` | `typedef sequence<long,4>` |
| `inner` | `@final` struct (nested leaf) |
| `nested` | `@appendable` struct: nested struct + `sequence<long,8>` + `string<64>` + enum |
| `choice_union` | `@final` union over `long` with a `default` arm |

Member/type names are load-bearing (Minimal hashes the member name; Complete
carries it verbatim), so the IDL is shared verbatim across all four
implementations.

## Result — 14-byte hash table (ZeroDDS vs vendors)

`CONSENSUS` = the vendors agree with one another (≥2 vendors, identical hash).
ZeroDDS matches **none** of them on **any** case.

| case | kind | ZeroDDS | CycloneDDS 11.0.1 | Fast DDS 3.6.1 | RTI 7.7.0 | vendor agreement |
|------|------|---------|----------|---------|-----|------|
| plain | MIN | `f88b6868…6431` | `deb922f0…51f6` | `deb922f0…51f6` | `ef5011f8…a7d5` | Cyclone=FastDDS |
| plain | CMP | `2ae126c3…fc93` | `7c0c6fab…0ea7` | `7c0c6fab…0ea7` | `7c0c6fab…0ea7` | **all 3** |
| appendable | MIN | `59971a4e…7652` | `3c916131…e2b6` | `3c916131…e2b6` | `f19a1a81…8874` | Cyclone=FastDDS |
| appendable | CMP | `bebdce0e…5b8c` | `27cb33d5…48fd` | `27cb33d5…48fd` | `27cb33d5…48fd` | **all 3** |
| mutable | MIN | `9245dc4f…c4da` | `81e76ce1…5831` | `81e76ce1…5831` | `32879900…5ec3` | Cyclone=FastDDS |
| mutable | CMP | `897363a0…8c06` | `447863b1…4866` | `447863b1…4866` | `447863b1…4866` | **all 3** |
| inner | MIN | `68430817…43f4` | `0c380bda…3c16` | `0c380bda…3c16` | `d47cb3d1…d9fb` | Cyclone=FastDDS |
| inner | CMP | `be1fb5c5…a9ed` | `f00388e0…8af8` | `f00388e0…8af8` | `f00388e0…8af8` | **all 3** |
| color_enum | MIN | `5580a1b0…09d6` | n/a* | `8dd1306b…abb6` | `e28caefd…8023` | split |
| color_enum | CMP | `1f420d45…b78b` | n/a* | `cf1aad4b…a1b5` | `c61e816d…0e36` | split |
| long_seq4_alias | MIN | `f9b0e158…b424` | n/a* | `37fa7f52…5893` | `aaf10a79…3124` | split |
| long_seq4_alias | CMP | `69052476…0267` | n/a* | `326bc924…30dac` | `af65abba…f15b` | split |
| nested | MIN | `7083c583…f4c3` | `a47f0e76…a333` | `d6ffe13c…8609` | `56cac11b…5713` | split |
| nested | CMP | `58baaac1…8d4a` | `9a07cbb7…6cfa` | `71b69c64…13ac` | `512f3e28…87b0` | split |
| choice_union | MIN | `b2162ca8…c9af` | `23e44a25…07a2` | `b31f1fca…8af9` | `08b7ae1d…41ad` | split |
| choice_union | CMP | `36c727aa…89f2` | `53fc8ab4…0177` | `57e6a4ca…0e15` | `d31cf2f0…f3c4` | split |

\* CycloneDDS only emits a TypeObject for top-level *topic* types (structs and
unions); the standalone `enum`/`alias` appear only as nested dependencies and
are not separately addressable as Cyclone topic descriptors. Their values are
recorded for Fast DDS and RTI.

**The decisive line:** for the four leaf **struct** types
(`plain`, `appendable`, `mutable`, `inner`), the **COMPLETE** hash is
**byte-identical across all three vendors** and **ZeroDDS differs**. CycloneDDS
and Fast DDS additionally produce **byte-identical serialized TypeObjects**
(55 B minimal / 80 B complete for `plain`); ZeroDDS produces 35 B / 116 B. This
is incontrovertible multi-vendor consensus that ZeroDDS is non-conformant.

### Root cause, read off the bytes (`plain`, MINIMAL)

```
ZeroDDS (35 B):     f1 50 | 01 00 00 00 00 00 | 02 00 00 00 | <members, flat> …
CycloneDDS/FastDDS  33 00 00 00 | f1 51 | 01 00 | 01 00 00 00 | 00 00 00 00 |
  (55 B):           23 00 00 00 | 02 00 00 00 | 0b 00 00 00 | … <member, XCDR2-framed>
```

1. **Wrong TypeKind octet.** ZeroDDS emits `0x50` for a struct. Per §7.2.2.2
   `TK_STRUCTURE = 0x51` (and `0x50` is `TK_ANNOTATION`). ZeroDDS's whole
   `TK_*` block from `TK_ANNOTATION` onward is shifted by one (annotation,
   structure, union, bitset). Cyclone/Fast DDS/RTI emit `0x51`.
2. **No XCDR2 DHEADERs.** The spec mandates XCDR2; the appendable
   member/sequence framing requires DHEADER length-prefixes (`33 00 00 00`
   outer, `01 00 00 00` struct body, `23 00 00 00` member-seq, `0b 00 00 00`
   per member). ZeroDDS serializes a flat XCDR1-style stream with none of
   these. (ZeroDDS's `MinimalTypeObject::to_bytes_le()` uses a plain
   `BufferWriter`, not the `.xcdr2()` writer.)
3. **Different COMPLETE padding/framing.** ZeroDDS's COMPLETE objects are
   *larger* than the vendors' (116 B vs 80 B for `plain`) because the
   member-detail/annotation members are serialized with fixed padding instead
   of the XCDR2 optional/appendable member framing.

See `../../../internal/idl-codegen/typeobject-byte-identity-findings.md` for the
full classification and proposed fixes.

### A note on RTI minimal hashes and RTI's serialized `.complete.bin`

* RTI's `DDS_TypeObjectV2_create_from_typecode` returns a **minimal** hash via
  a different derivation than Cyclone/Fast DDS, so RTI's MINIMAL hashes do not
  join the Cyclone=FastDDS consensus. The **COMPLETE** hashes do agree across
  all three — that is the authoritative interop signal recorded above.
* RTI's `vendor/rti/*.complete.bin` are produced by `DDS_TypeObjectV2_serialize`
  and carry RTI's own outer framing/encapsulation (e.g. 104 B for `plain`
  vs the 80 B canonical body). They are committed as-is for provenance; the
  byte-for-byte canonical-body comparison is done with the **CycloneDDS/Fast DDS**
  goldens, and the cross-vendor agreement is asserted on the **hash**.

## Reproduction

### ZeroDDS side

```
cargo run --bin typeobject_proof            # prints bytes + 14-byte hashes, writes goldens/
cargo run --bin typeobject_proof -- --check # byte/hash-diff vs the committed vendor goldens
```

The harness builds each TypeObject with `zerodds_types::builder` exactly as
ZeroDDS registers it, serializes via `zerodds_types` and computes the hash via
`zerodds_types::hash::compute_{minimal,complete}_hash` (MD5, first 14 bytes).

### Vendor side (recorded on a host with the three stacks; versions above)

* **CycloneDDS** — `idlc typeobject.idl` emits `typeobject.c` containing, per
  type, a `TYPE_INFO_CDR_*` blob (the `TypeInformation`, holding the hashes)
  and a `TYPE_MAP_CDR_*` blob (the `TypeMapping`, holding the serialized
  TypeObjects). `extractors/cyclone_extract.py` parses the `TypeMapping`,
  re-frames each TypeObject as `DHEADER||body`, and verifies `MD5(...)[0:14]`
  equals the hash Cyclone stored (it does, for all types — the MD5-framing
  proof). Output → `vendor/cyclonedds/`.
* **Fast DDS** — `fastddsgen typeobject.idl` emits
  `typeobjectTypeObjectSupport.cxx` with `register_*_type_identifier()`
  functions. `extractors/fastdds_extract.cpp` calls them, fetches each
  Minimal/Complete TypeObject from `type_object_registry()`, serializes it with
  fastcdr (XCDR v2, LE), and reads the 14-byte hash from the registered
  `TypeIdentifier`. Output → `vendor/fastdds/`.
* **RTI Connext** — `rtiddsgen -language C typeobject.idl` emits the type
  plugins. `extractors/rti_extract.c` takes each `DDS_TypeCode`, calls
  `DDS_TypeObjectV2_create_from_typecode` (which returns the complete +
  minimal `EquivalenceHash`es) and `DDS_TypeObjectV2_serialize`. Output →
  `vendor/rti/`.

Build lines are in the header comment of each extractor. Vendor goldens are
committed under `vendor/<vendor>/` so the `--check` run is reproducible without
the vendor toolchains.

## Vendor versions

| vendor | version |
|--------|---------|
| CycloneDDS | 11.0.1 (`idlc`/`libddsc.so.11.0.1`) |
| eProsima Fast DDS | 3.6.1 (`libfastdds.so.3.6.1.0`), fastddsgen via OpenJDK 21 |
| RTI Connext DDS | 7.7.0 (`rtiddsgen` 4.7.0, `x64Linux4gcc8.5.0`) |
| ZeroDDS | this tree (`crates/types`) |
