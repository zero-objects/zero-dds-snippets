# Cross-PSM interop — canonical samples (exact values; identical across all PSMs)

**The wire is the wire: there is ONE golden per (feature, representation), not one
per language.** Two tiers:

* **Tier 1 — cross-vendor (the wire truth + policy):** run `features.idl` through
  every implementation (Cyclone, RTI, FastDDS, OpenDDS, ZeroDDS), document where
  they diverge, and from that decide ZeroDDS's *default* wire (its default feature
  set). Vendors genuinely disagree on some constructs — see
  `proofs/endianness/vendor-{cyclone,fastcdr,rti}/` and
  `internal/idl-codegen/sx2-vendor-extensibility-divergence.md` (e.g. unannotated
  nested-struct default extensibility: Cyclone=final/48 B, RTI=52 B, FastDDS=56 B;
  ZeroDDS default = appendable/56 B = FastDDS, `--default-extensibility final` = 48 B
  = Cyclone).

* **Tier 2 — intra-ZeroDDS (language conformance):** ONE golden = ZeroDDS's chosen
  default wire, `goldens/<feature>.golden.bin`. **Every** ZeroDDS language binding
  (rust/cpp/python/java/ts/c/csharp) must (a) encode these exact values to bytes
  **byte-identical to the golden**, and (b) decode the golden back to the exact
  values. The languages *follow* the golden; they do not each own one.

  The per-language encoder outputs (`<feature>.<lang>.bin`) are **ephemeral**
  verification artifacts (git-ignored), produced by `verify.sh` and `cmp`'d against
  the golden. `goldens/combo.golden.bin` is the same model for `combo::Telemetry`.

The golden wire is XCDR2-LE, default representation; the rust PSM (running through
the cross-vendor-validated `zerodds-cdr` core) regenerates it.

## feat::WStr  (golden `wstr.<lang>.bin`)
- label (wstring<16>) = `"café"`  → the 4 code points c, a, f, é (U+00E9)
- text  (wstring)     = `"日本語\U0001F389"` → 日 本 語 🎉 (🎉 = U+1F389, surrogate pair D83C DF89 in UTF-16)
  (UTF-16LE on the wire: uint32 byte-length, then the code units, no terminator)

## feat::Mut  (golden `mut.<lang>.bin`)  — @mutable, EMHEADER per member
- a (@id 10, long)      = 1000000
- b (@id 20, double)    = 2.5
- c (@id 30, string<8>) = `"ok"`

## feat::MutNest  (golden `mutnest.<lang>.bin`)  — nested @mutable + sequence<@mutable>
- MutLeaf is `@mutable { @id 1 long u; @id 2 double v; }`.
- tag  (@id 10, long)              = 9
- leaf (@id 20, MutLeaf)           = { u=100, v=1.25 }
- list (@id 30, sequence<MutLeaf>) = [ {u=1,v=0.5}, {u=2,v=0.25} ]
- EMHEADER LengthCode rule (XTypes 1.3 §7.4.3.4.2): a member whose body begins
  with a 4-byte length word REUSES it as the NEXTINT via **LengthCode 5** (no
  separate NEXTINT). So `tag` (primitive) = LC2, `leaf` (nested @mutable DHEADER)
  = LC5, `list` (non-primitive sequence DHEADER) = LC5. Result = 100 bytes,
  byte-identical to FastDDS's universal-LC5 form. A @final nested struct (no
  DHEADER) and a `sequence<primitive>` (bare count) would stay LC4.

## feat::OuterKey  (golden `outerkey.<lang>.bin`)  — nested-struct @key
- NestedKey is `@nested @final { @key long hi; @key long lo; }`.
- k       (@key NestedKey) = { hi=0x01020304, lo=0x05060708 }
- payload (long)           = 999
- On the wire (XCDR2-LE) OuterKey is a plain @final aggregate: hi, lo, payload —
  12 bytes. The `@key` annotation governs the KeyHash (PID_KEY_HASH), proven
  byte-for-byte against Cyclone/RTI/FastDDS in `proofs/keyhash/` (recursive
  key expansion, XTypes 1.3 §7.6.8 step 3), not these payload bytes.

## feat::Bits  (golden `bits.<lang>.bin`)
- perm (bitmask Perm, holder uint32) = READ | EXEC  = bit0 | bit2 = 0x00000005
- flags (bitset Flags, holder uint8: kind<3> @ shift 0, prio<5> @ shift 3)
  - kind = 5 (0b101), prio = 20 (0b10100)  → holder = (20<<3) | 5 = 0xA5

## feat::Tree  (golden `tree.<lang>.bin`)  — recursion
- root: value=1, kids=[ {value=2, kids=[ {value=4, kids=[]} ]}, {value=3, kids=[]} ]

## feat::MapEnum  (golden `mapenum.golden.bin`)  — @bit_bound enum + map-of-struct + sequence-of-union
- `@bit_bound(16) enum Hue { H_RED, H_GREEN, H_BLUE }`; `union Sel switch(long)
  { case 1: Pt p; case 2: long n; default: octet z; }` (@appendable).
- h (Hue)               = H_BLUE  → 2-byte holder (NOT 4)
- m (map<long, Pt>)     = { 3: {x=11, y=12} }
- sels (sequence<Sel>)  = [ Sel(n=9) ]   (discriminator 2)
- 52 bytes. Exercises three combinations that each surfaced a real backend bug:
  the map DHEADER must be 4-aligned even though it follows the 2-byte enum (cpp/c
  map-align fix); each @appendable union element carries its OWN DHEADER (c +
  python union-element fix). XCDR2-LE layout: outer DHEADER(48), h(0x0002)+pad,
  map DHEADER(20) + count + key + Pt DHEADER + x + y, sels DHEADER(16) + count +
  Sel DHEADER(8) + disc(2) + n(9).

## feat::Arr  (golden `arr.<lang>.bin`)
- grid (long[2][3]) = [[10,11,12],[13,14,15]]  (row-major)
- shape (Pt[2])     = [ {x=1,y=2}, {x=3,y=4} ]

## feat::Prim  (golden `prim.<lang>.bin`)  — extremes
- i8=-128, u8=255, i16=-32768, u16=65535, i32=-2147483648, u32=4294967295,
- i64=-9223372036854775808, u64=18446744073709551615,
- f32=3.5 (exact), f64=-1234.5 (exact), b=true(1), o=0xAB, ch='Z' (0x5A)

## feat::Money  (golden `fixed.golden.bin`)  — fixed<P,S> CORBA-BCD decimal
- price (fixed<5,2>) = `123.45`  → BCD `12 34 5c` (3 octets)
- qty   (fixed<4,0>) = `1234`    → BCD `01 23 4c` (3 octets)
- `@appendable`, so the wire is DHEADER(6) + price(3) + qty(3) = **10 bytes**:
  `06 00 00 00 12 34 5c 01 23 4c`. fixed is align-1, no length prefix, endian-
  independent — the BCD octets are the CORBA/GIOP §9.3.2.7 form (oracle-validated
  against JacORB 3.9 / omniORB 4.3). It is NOT an XTypes type (no TypeObject), so
  CycloneDDS/Fast DDS reject it; ZeroDDS carries it across all seven bindings.

## Conformance criterion (per feature, per language)
For every feature F and every language L in {rust,cpp,python,java,ts,c,csharp}:
1. `L.encode(canonical(F))` is **byte-identical** to `goldens/F.golden.bin`.
2. `L.decode(goldens/F.golden.bin)` reconstructs `canonical(F)` exactly.
`verify.sh` runs all 7 languages and checks both directions against the single
golden. There is no per-language golden to drift; a language that regresses fails
(1) against the golden directly.
