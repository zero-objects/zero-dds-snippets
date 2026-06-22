# Cross-PSM interop — canonical samples (exact values; identical across all PSMs)

Each PSM encodes these EXACT values for `features.idl` (XCDR2-LE, default repr).
The encoded bytes must be byte-identical across all 7 languages; each PSM must
also decode every other PSM's golden. Goldens: `goldens/<feature>.<lang>.bin`.

`combo` (combo::Telemetry, the existing one) stays as is (golden `<lang>.bin`).

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

## feat::Arr  (golden `arr.<lang>.bin`)
- grid (long[2][3]) = [[10,11,12],[13,14,15]]  (row-major)
- shape (Pt[2])     = [ {x=1,y=2}, {x=3,y=4} ]

## feat::Prim  (golden `prim.<lang>.bin`)  — extremes
- i8=-128, u8=255, i16=-32768, u16=65535, i32=-2147483648, u32=4294967295,
- i64=-9223372036854775808, u64=18446744073709551615,
- f32=3.5 (exact), f64=-1234.5 (exact), b=true(1), o=0xAB, ch='Z' (0x5A)

## Convergence criterion (per feature)
all 7 `goldens/<feature>.*.bin` share one SHA-256 AND every PSM decodes every
other PSM's golden for that feature. Reference = the rust golden (runs through
the cross-vendor-validated `zerodds-cdr` core); the other 6 conform to it.
