# Endianness × XCDR-version wire proof

This example proves that ZeroDDS serializes the OMG **XTypes 1.3** wire format
correctly across **both byte orders** (little- and big-endian) and **both CDR
versions** (XCDR1 = `PLAIN_CDR`, XCDR2 = `PLAIN_CDR2`), and that the resulting
bytes are **byte-identical to CycloneDDS and eProsima Fast DDS** wherever the
spec is unambiguous.

The original interop corpus (`../../_interop/`) pins one representation only:
**XCDR2, little-endian**. That is the form a ZeroDDS DataWriter emits by default
on x86/ARM, and all seven ZeroDDS language backends already converge on it
byte-for-byte. This proof widens the check to the three representations that the
default path never exercises:

| repr       | encapsulation id | CDR version | max alignment | extensibility framing |
|------------|------------------|-------------|---------------|------------------------|
| `xcdr2-le` | `0x0011` LE      | XCDR2       | **4**         | DHEADER (appendable), EMHEADER (mutable) |
| `xcdr2-be` | `0x0010` BE      | XCDR2       | **4**         | same, big-endian |
| `xcdr1-le` | `0x0001` LE      | XCDR1       | **8**         | none (appendable plain), PL_CDR (mutable) |
| `xcdr1-be` | `0x0000` BE      | XCDR1       | **8**         | same, big-endian |

(The goldens here are the **bare CDR body** — the 4-byte RTPS encapsulation
header that carries the id above is stripped, on both sides, so the comparison
is over the payload only.)

## Why XCDR1 and big-endian matter

* **XCDR1 vs XCDR2 alignment.** XCDR2 (§7.4.1.1.1) caps primitive alignment at
  4: a `double`/`int64`/`uint64` aligns to 4, **not** 8. XCDR1 (classic
  `PLAIN_CDR`) keeps the full max-alignment of 8. The same struct therefore has
  a *different byte layout* depending on the version — getting the alignment cap
  wrong silently corrupts 64-bit members on the wire. This was the historical
  XTypes `align(8)`-vs-`min(sizeof,4)` interop bug; this proof exercises it
  directly on a struct (`Prim`) that carries `int64`/`uint64`/`double`.
* **XCDR2 carries a DHEADER, XCDR1 does not.** An `@appendable` struct gets a
  4-byte DHEADER (delimited length) under XCDR2 but is encoded *plain* under
  XCDR1. An `@mutable` struct uses EMHEADER member framing under XCDR2 but the
  PID-based `PL_CDR` parameter list under XCDR1.
* **Big-endian** flips the byte order of *every* primitive, *every* length
  prefix, *and* every UTF-16 wstring code unit. A codec that only ever runs
  little-endian (because every dev box is x86/ARM) can hide a byte-swap bug in
  any of those three places.

## The samples

Seven topic types from `features.idl` (identical values to
`../../_interop/CANONICAL.md`):

* `wstr`  — bounded + unbounded `wstring` (UTF-16; includes an astral 🎉).
* `mut`   — `@mutable` struct (EMHEADER / PL_CDR member framing).
* `bits`  — `bitmask` + `bitset`.
* `tree`  — self-recursive `sequence<Tree>`.
* `arr`   — multi-dim array + array-of-struct.
* `prim`  — every integer type at min/max + exact floats (the 64-bit alignment
            probe).
* `mapenum` — `@bit_bound(16)` enum + `map<long,Pt>` (map-of-struct) +
            `sequence<Sel>` (sequence-of-`@appendable`-union). Probes collection
            DHEADER alignment after a sub-4-byte member, and the narrow-enum
            width on the wire (see ⁷). Cyclone can't parse `map`, so it is
            FastDDS-anchored only.

## Result table (repr × vendor)

`MATCH` = byte-identical to ZeroDDS. `DIFF` = bytes differ (see `FINDINGS.md`).
`n/a` = the vendor's code generator cannot emit that type (documented below).

| feature | xcdr2-le | xcdr2-be | xcdr1-le | xcdr1-be | vs Cyclone | vs FastDDS |
|---------|----------|----------|----------|----------|------------|------------|
| prim    | ✓ ✓      | ✓ ✓      | ✓ ✓      | ✓ ✓      | **MATCH (4/4)** | **MATCH (4/4)** |
| bits    | ✓        | ✓        | ✓        | ✓        | n/a        | **MATCH (4/4)** |
| arr     | ✓        | ✓        | ✓        | ✓        | MATCH xcdr1; DIFF xcdr2¹ | **MATCH (4/4)**¹ |
| mut     | ✓        | ✓        | ✓        | ✓        | n/a        | MATCH xcdr2; DIFF xcdr1² |
| wstr    | ✓        | ✓        | ✓        | ✓        | MATCH LE; DIFF BE³ | DIFF (count)⁴ |
| tree    | ✓        | ✓        | ✓        | ✓        | n/a⁵       | n/a⁵       |
| mapenum | ✓        | ✓        | ✓        | ✓        | n/a⁶       | MATCH LE; DIFF BE⁷ |

The `✓` columns mean *the ZeroDDS Rust reference produced that representation
through the same codec the DataWriter uses, and it round-trips back to the
canonical value* (except the two decode gaps noted in `FINDINGS.md`, which are
encode-correct). The XCDR2-LE column is additionally byte-identical to all seven
ZeroDDS language backends (see `../../_interop/goldens/`).

¹ `arr`/xcdr2 — **extensibility-default**, not an endianness/alignment bug.
  `struct Pt` is unannotated, so its extensibility is the compiler default. Per
  XTypes §7.3.1.2.1 that default is `@appendable`; ZeroDDS now follows the spec
  (and fastddsgen), wrapping each `shape[2]` array element in its own DHEADER →
  56 bytes, **byte-identical to FastDDS in all four reprs**. CycloneDDS instead
  defaults an unannotated struct to `@final` (no per-element DHEADER → 48 bytes),
  so it diverges on xcdr2 only (xcdr1 has no DHEADER either way → MATCH). The two
  vendor camps are bridged by configuration: `idlc --default-extensibility final`
  (alias `-de final`) makes ZeroDDS emit Cyclone's 48-byte form. (This inverts an
  earlier revision of this proof, when ZeroDDS still defaulted to `@final`.)

² `mut`/xcdr1 vs FastDDS — **ZeroDDS gap**: the `@mutable` XCDR1 path emits the
  members *plain-positionally* instead of as a `PL_CDR` PID parameter list. See
  `FINDINGS.md` #2. (XCDR2-LE/BE — the default DataWriter path — are correct and
  byte-identical to FastDDS.)

³ `wstr`/BE vs Cyclone — Cyclone does **not** byte-swap the UTF-16 code units in
  a big-endian stream (it emits machine-native LE units); ZeroDDS and FastDDS
  both byte-swap them, which is the correct CDR behaviour for a 2-byte
  primitive. This is a CycloneDDS quirk, not a ZeroDDS bug. See `FINDINGS.md` #3.

⁴ `wstr` vs FastDDS — FastDDS writes the wstring length as a **code-unit count**;
  ZeroDDS *and CycloneDDS* write it as an **octet count** (classic-CDR rule). A
  genuine, underspecified cross-vendor disagreement; ZeroDDS is on the Cyclone
  side. See `FINDINGS.md` #4.

⁵ `tree` — **neither** Cyclone's `idlc` nor `fastddsgen` can generate a
  self-recursive `sequence<Tree>` (fastddsgen errors; Cyclone idlc emits empty
  files). ZeroDDS encodes it and round-trips it in all four reprs, and all seven
  ZeroDDS backends agree on the XCDR2-LE bytes, but there is no DDS-vendor oracle
  to byte-compare against, for a structural reason on the vendor side.

⁶ `mapenum` vs Cyclone — **n/a**: Cyclone's `idlc` rejects `map<K,V>` (an XTypes
  extension it does not parse), so there is no Cyclone oracle for this type. The
  full structural framing is anchored against FastDDS instead.

⁷ `mapenum` vs FastDDS — the map-of-struct DHEADER alignment, the `@appendable`
  `Pt` value DHEADER, and the `@appendable` `Sel` union element DHEADER are all
  **byte-identical to FastDDS** (the structural framing this proof exists to
  pin). The xcdr2-**le** and xcdr1-**le** forms MATCH outright; the two **be**
  forms differ at exactly one place — the `@bit_bound(16) enum Hue` member.
  ZeroDDS emits it as a **2-byte** holder (the §7.4.5.1 narrow-bound width, which
  CycloneDDS also uses); FastDDS emits it as a **4-byte** int32 regardless of the
  bit-bound. The little-endian reprs hide this (the low two bytes coincide and
  the following member is 4-aligned either way); big-endian exposes it. This is
  the same `@bit_bound`-enum-width decision already recorded for `bits`/enums —
  ZeroDDS is on the spec/Cyclone side. Total length is 52 / 32 bytes on both
  sides; only the enum's two extra bytes move.

## Vendor versions & how they were configured

See `vendor-src/VENDOR-VERSIONS.txt`. Summary:

* **CycloneDDS 0.11.0** (`libddsc.so.11.0.1`). Oracle drivers
  (`vendor-src/cyclone_*_oracle.c`) call `dds_ostreamLE_init` /
  `dds_ostreamBE_init` with `xcdr_version` 1 or 2 and `dds_stream_writeLE/BE`
  against the `idlc -l c` topic descriptor — i.e. Cyclone's own CDR stream, with
  the version + endianness selected explicitly.
* **eProsima Fast DDS 3.6.1 / Fast CDR 2.3.5**, types generated with
  **fastddsgen v4.3.0**. The oracle driver (`vendor-src/fastdds_oracle.cpp`)
  builds an `eprosima::fastcdr::Cdr(buffer, endianness, CdrVersion::XCDRv1|v2)`
  and calls the generated `serialize()` — so the XTypes framing (DHEADER /
  EMHEADER / PL_CDR) is whatever Fast CDR itself produces for that version.

No vendor binaries were modified. Fast CDR leaves alignment padding bytes
uninitialized; the oracle zeroes its scratch buffer before serializing so the
padding compares deterministically (ZeroDDS always zero-fills padding).

## Reproduce

ZeroDDS side (local, needs the `zerodds` workspace checked out alongside):

```sh
make goldens     # cargo run -p endian-proof-rust ENCODE  -> goldens/
make roundtrip   # cargo run … ROUNDTRIP  (self-consistency of the codec)
```

Vendor side (on a host with CycloneDDS + Fast DDS + fastddsgen installed):

```sh
make vendors     # uploads features.idl, generates+builds+runs both oracles,
                 # pulls goldens into vendor-cyclone/ and vendor-fastcdr/
```

Then the table + diff detail:

```sh
python3 compare.py
```

The committed `goldens/`, `vendor-cyclone/`, and `vendor-fastcdr/` directories
are the captured proof; `compare.py` reproduces the table from them offline.

### Round-trip self-consistency (`make roundtrip` = 28/28 OK)

`make roundtrip` re-decodes each freshly-encoded buffer through the `CdrDecode`
trait and asserts equality. All 28 (7 features × 4 reprs) now pass.

This previously had six big-endian decode failures — all encode-correct (the
bytes matched the vendors), but the **decode** side hardcoded little-endian in
two generated paths:

* the inline `wstring` decode read UTF-16 units with `from_le_bytes` regardless
  of stream order (`wstr`/`xcdr2-be`, `wstr`/`xcdr1-be`); and
* the `@mutable` member-body sub-reader was built little-endian instead of
  inheriting the parent stream order, so an LC5 string's reused length word read
  back swapped (`mut`/`xcdr2-be`).

Both were fixed in the Rust IDL backend (`crates/idl-rust`, commit
*@mutable + wstring decoders must honor the stream byte order*) and are now
gated by an `edge_roundtrip` test that round-trips both constructs in both byte
orders.

### Big-endian support across the seven backends (audited)

This proof exercises BE through the ZeroDDS **Rust** PSM only. Auditing the other
backends' codecs shows BE *decode* is a **deliberate design boundary, not a bug
cluster** — the typed `decode(bytes)` entry point is little-endian-only almost
everywhere, because the bare CDR body carries no byte-order mark and the
encapsulation header (which does) is stripped above this layer:

| backend | BE encode | BE decode (typed entry) | note |
|---------|-----------|-------------------------|------|
| Rust `CdrEncode`/`CdrDecode` trait | ✓ | ✓ | reader carries the byte order; the two bugs above were here, now fixed + gated |
| Rust `DdsType` (DCPS-facing) | LE | LE | `decode(bytes)` builds an LE reader |
| C++ | partial (`write_be` helpers) | LE | `read_le_origin` |
| C# | ✓ (`Encode(.., EndianMode)`) | LE | `Decode(bytes)` hardcodes an LE `Xcdr2Reader`; the reader itself is endian-complete |
| TypeScript | ✓ (`encode(.., endian)`) | LE | `decode(bytes)` takes no endian arg |
| Python | LE-only | LE-only | `cdr.py` is an explicit little-endian codec |
| Java | LE-only | LE-only | `ByteBuffer.order(LITTLE_ENDIAN)` |

So the only genuine BE *bug* was the Rust `CdrDecode` sub-structure path ignoring
the byte order its reader already carried; that is fixed. Wiring BE **decode**
end-to-end through every typed entry point (parse the encapsulation byte-order
bit → thread it into the generated decoder of all seven backends) is a real but
**unwired feature**, not a latent defect — no big-endian DDS peer exists on
current hardware to exercise it. It is recorded here as a known boundary, not
silently implied as covered.
