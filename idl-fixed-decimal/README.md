# `fixed<P,S>` decimal on the wire — the CORBA type DDS forgot

IDL has an exact decimal type, `fixed<P,S>` (`P` total digits, `S` after the
point). Money and engineering quantities need it: a `double` cannot represent
`123.45` exactly. ZeroDDS carries `fixed` on the wire across its bindings — and
its bytes are the **CORBA/GIOP §9.3.2.7 packed-BCD** form, byte-identical to what
JacORB and omniORB emit.

```idl
// fixed.idl
module shop {
  @final struct Price {
    @key  long       sku;
          fixed<5,2> amount;   // 123.45 -> 12 34 5c   (3 octets)
  };
  @appendable struct Order {
          long       id;
          fixed<4,0> qty;      // 1234   -> 01 23 4c
  };
};
```

## The wire — packed BCD, no length prefix, endian-independent

A `fixed<P,S>` value is `(P+2)/2` octets of binary-coded decimal: the `P` digit
nibbles most-significant-first, then a sign nibble (`0xC` positive, `0xD`
negative), with a leading `0x0` pad nibble when `P+1` is odd. The byte count is
static (from `P`), so there is **no length prefix**, and a BCD octet array reads
the same big- or little-endian.

| value | type | BCD octets |
|---|---|---|
| `123.45` | `fixed<5,2>` | `12 34 5c` |
| `1234`   | `fixed<4,0>` | `01 23 4c` |
| `-1.50`  | `fixed<6,2>` | `00 00 15 0d` |

So the full XCDR2-LE encoding of `Order{ id=7, qty=1234 }` (`@appendable`, so a
DHEADER frames the body) is:

```
07 00 00 00   DHEADER = 7 (body length)
07 00 00 00   id = 7
01 23 4c      qty = 1234   (BCD)
```

and `Price{ sku=7, amount=123.45 }` (`@final`, no DHEADER) is `07 00 00 00 12 34 5c`.

## Cross-vendor — anchored to real CORBA ORBs

These are not bytes ZeroDDS invented. The decimal codec
(`crates/cdr/src/fixed.rs`) is **byte-validated against JacORB 3.9 and omniORB
4.3**: a `fixed<5,2>` of `123.45` is `12 34 5c`, a `fixed<4,0>` of `1234` keeps
its most-significant digit as `01 23 4c` — the exact octets above. And every
ZeroDDS binding that carries `fixed` produces the *same* bytes: the Rust and C++
PSMs encode `Order`/`Price` to the vectors shown here, proven by

- `crates/idl-rust/tests/wire_roundtrip.rs::wire_fixed_member_golden_bytes`
- `crates/idl-cpp/tests/clang_roundtrip.rs::fixed_member_roundtrips_through_clang`

both asserting the byte vectors above across `@final`, `@appendable` and
`@mutable` extensibilities.

## Why no other DDS stack carries it

`fixed` is a **CORBA type**. It is absent from the DDS-XTypes 1.3 type system —
there is no `TK_FIXED`, so a `fixed` member has no `TypeIdentifier`/`TypeObject`,
and the XTypes type-matching machinery cannot describe it. The mainstream DDS IDL
compilers reject it outright in a topic:

| compiler | `struct S { fixed<5,2> p; }` |
|---|---|
| CycloneDDS `idlc` | `syntax error` at the member |
| Fast DDS `fastddsgen` | parser `NullPointerException` |
| ZeroDDS `idlc` | **compiles, encodes, decodes** |

So this is a ZeroDDS differentiator: a `fixed`-bearing topic round-trips
ZeroDDS↔ZeroDDS in any wired binding, and bridges ZeroDDS-DDS ↔ ZeroDDS-CORBA via
the same BCD wire — useful when migrating a CORBA system that already models
money as `fixed` onto DDS. The type still emits **no TypeObject** (it has none),
so an XTypes-strict foreign reader simply will not match it — exactly as the spec
implies.

## Running it

```
# generate the bindings
zerodds-idlc fixed.idl --rust -o gen_rust
zerodds-idlc fixed.idl --cpp  -o gen_cpp

# the executable proofs live in-tree (see the two tests linked above):
cargo test -p zerodds-idl-rust --test wire_roundtrip -- --ignored wire_fixed
cargo test -p zerodds-idl-cpp  --test clang_roundtrip -- --ignored fixed_member
```

Part of **[ZeroDDS](https://zerodds.de)**.
