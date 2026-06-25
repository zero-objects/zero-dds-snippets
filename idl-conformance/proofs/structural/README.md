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

## Why only Fast DDS — `map<>` is not portable IDL

`map<>` could **not** be anchored against CycloneDDS or RTI Connext, because
neither of their IDL compilers parses it:

| compiler | `map<long, Pt>` |
|---|---|
| Fast DDS `fastddsgen` 4.x | ✅ accepted → `std::map` |
| ZeroDDS `idlc` | ✅ accepted |
| CycloneDDS `idlc` | ❌ `syntax error` at the `map` member |
| RTI `rtiddsgen` 4.7 | ❌ `no viable alternative at input '<'` (no `-map`/escape flag) |

This is a **vendor IDL-frontend limitation, not a wire question** — `map<>` has a
well-defined XCDR2 encoding (a length-prefixed sequence of key/value pairs, which
is exactly what the Fast DDS / ZeroDDS bytes above encode). It does mean a
map-bearing topic can only be IDL-compiled — and thus statically anchored — against
Fast DDS and ZeroDDS among these four stacks. The non-map structural features
(`@bit_bound` enum, union-of-struct, sequence-of-union) are independently anchored
across more vendors in `../endianness` and `../typesystem`.

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
