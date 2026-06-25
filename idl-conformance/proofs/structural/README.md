# Structural-feature vendor anchor — `feat::MapEnum`

The `_interop` corpus' structural feature combines three constructs that each
surfaced a real backend bug in their own right, in **one** `@appendable` struct
([`MapEnum.idl`](MapEnum.idl), a subset of `_interop/features.idl`):

```idl
@bit_bound(16) enum Hue { H_RED, H_GREEN, H_BLUE };
@appendable union Sel switch(long) { case 1: Pt p; case 2: long n; default: octet z; };
@appendable struct MapEnum {
  Hue            h;      // @bit_bound(16) enum  -> 2-byte holder
  map<long, Pt>  m;      // map of struct value
  sequence<Sel>  sels;   // sequence of @appendable union (-of-struct)
};
```

The canonical sample (`CANONICAL.md`): `h = H_BLUE`, `m = {3: {x=11, y=12}}`,
`sels = [Sel(n=9)]` → **52 bytes** XCDR2-LE.

## Vendor anchor — Fast DDS is byte-identical

[`vendor-src/fastdds_mapenum_oracle.cpp`](vendor-src/fastdds_mapenum_oracle.cpp)
serializes the canonical sample with the `fastddsgen`-generated `serialize()`
(FastCDR, `CdrVersion::XCDRv2`, little-endian, **bare body, no encapsulation**):

```
$ fastddsgen -replace MapEnum.idl
$ g++ -std=c++17 -I. -I<fastdds>/include fastdds_mapenum_oracle.cpp \
      MapEnumPubSubTypes.cxx MapEnumTypeObjectSupport.cxx -L<fastdds>/lib -lfastcdr -lfastdds -o oracle
$ ./oracle && cmp out/mapenum.xcdr2-le.fastdds.bin ../../_interop/goldens/mapenum.golden.bin
# (no output = byte-identical)
```

**Result: byte-identical.** Fast DDS, an independent implementation, produces the
exact 52 bytes ZeroDDS chose as its golden — and the corpus' Tier-2 already shows
all seven ZeroDDS language bindings (rust/cpp/python/java/ts/c/csharp) encode the
same golden. So the `@bit_bound(16)` enum holder, the map-of-struct, and the
`sequence<@appendable union>` are wire-anchored against an external vendor, not
just self-consistent. `out/mapenum.xcdr2-le.fastdds.bin` is the recorded artifact.

The `Sel` union-of-struct is anchored **transitively**: it is the element type of
`sels`, so the byte match validates the per-element union DHEADER + discriminator
+ branch encoding too.

## `map<>` qualified — who compiles it, and the XCDR2 DHEADER rule

`map<>` (IDL 4.2 §7.4.13.4 / DDS-XTypes 1.3 `TK_MAP`) is a standard collection,
but IDL-frontend support is uneven. Tested end-to-end (parse → generate type →
**compile**), not just "did the parser accept it":

| stack | IDL compiler | `map<>` end-to-end | note |
|---|---|:--:|---|
| ZeroDDS | `idlc` | ✅ | — |
| Fast DDS | `fastddsgen` 4.x | ✅ | → `std::map`; byte-anchored above |
| **OpenDDS** | `tao_idl` + `opendds_idl` 3.34 | ✅ | compiles + serializes; **second anchor** (below) |
| CycloneDDS | `idlc` 11.0.1 | ❌ | parser: `syntax error` at the `map` member (no flag) |
| RTI Connext | `rtiddsgen` 4.7.0 | ❌ | parser: `no viable alternative at input '<'` — both `-standard` modes, bounded `map<K,V,N>` too |

So **three of five** compile `map<>` (ZeroDDS, Fast DDS, OpenDDS); CycloneDDS and
RTI reject it at the parser, which is a frontend limitation, not a wire question.
*(Caveat: the **full** `MapEnum` does not build on OpenDDS — but because its
`@bit_bound` enum annotation is unknown to `tao_idl`, not because of the map; a
plain `map<long,Pt>` / `map<long,long>` struct compiles cleanly.)*

### The DHEADER rule — and a ZeroDDS bug it exposed

The cross-vendor diff also corrected a real ZeroDDS encoding bug. Per XCDR2
(§7.4.3.5), a collection carries a DHEADER **only when its element is
non-primitive** — same as a sequence. So:

- `map<long, Pt>` (struct value) → DHEADER. ZeroDDS, Fast DDS agree (the 52-byte
  `MapEnum` golden above is byte-identical).
- `map<long, long>` (primitive value) → **no DHEADER**. Here ZeroDDS originally
  over-emitted a spurious 4-byte DHEADER, while Fast DDS **and** OpenDDS both omit
  it. [`mapbare/`](mapbare) records the three encoders for `map<long,long> = {7:42}`;
  after the fix (`crates/cdr` keys the map DHEADER on `K::IS_PRIMITIVE &&
  V::IS_PRIMITIVE`) all three are byte-identical:

  ```
  ZeroDDS / Fast DDS / OpenDDS :  01 00 00 00 07 00 00 00 2a 00 00 00   (count, key=7, value=42)
  ```

That bug was invisible to the `MapEnum` golden (which uses a *struct*-valued map,
the path that correctly keeps a DHEADER) — it only showed once `map<primitive,
primitive>` was diffed against two independent vendors.

**Follow-up — done, and it was systemic.** A `map<long,long>` golden
([`_interop/goldens/mapprim.golden.bin`](../../_interop/goldens/mapprim.golden.bin),
the `mapprim` feature) was added to the Tier-2 corpus so all seven ZeroDDS language
bindings encode it byte-for-byte. That golden revealed the spurious DHEADER was
**not** just the Rust core — every one of the six non-Rust backends carried it in
its own generated map codec (each emits the collection DHEADER inline). All six
were fixed to gate the DHEADER on `K::IS_PRIMITIVE && V::IS_PRIMITIVE` (and the
`@mutable` case to pick EMHEADER LengthCode 4 instead of the shared-DHEADER LC5):
TypeScript, C++, C, the Python runtime, Java and C#. The same diff also turned up a
latent Java `Xcdr2Writer.align()` bug — it did not cap the boundary at the
representation's max-alignment, so 8-byte primitives aligned to 8 instead of 4
under XCDR2. The corpus now reports *ALL LANGUAGES CONFORM TO THE GOLDEN* for both
`mapenum` (struct-valued, keeps its DHEADER) and `mapprim` (primitive-valued, omits
it) across all seven bindings.

The non-map structural features (`@bit_bound` enum, union-of-struct,
sequence-of-union) are independently anchored across more vendors in
`../endianness` and `../typesystem`.

## Cross-middleware — live over the wire (ZeroDDS ↔ Fast DDS)

Beyond the static byte-anchor, the type also flows **live**, both directions,
best-effort reader, native RTPS 2.5 ([`demo/`](demo)):

```
ZeroDDS  → Fast DDS : 60/60  h=H_BLUE m.size=1 sels.size=1 integrity=OK
Fast DDS → ZeroDDS  : 58/58  h=H_BLUE m.len=1  sels.len=1  integrity=OK
```

Each side rebuilds the canonical sample from the decoded fields and checks every
value (`h == H_BLUE`, `m[3] == {11,12}`, `sels[0]` is the `n=9` branch). So the
map-of-struct + `@bit_bound` enum + union-of-struct round-trips across two
independent middleware stacks, not just byte-matches a recorded golden.
`demo/` holds the ZeroDDS (Rust, generated) and Fast DDS (C++) pub/sub.

Part of **[ZeroDDS](https://zerodds.de)**.
