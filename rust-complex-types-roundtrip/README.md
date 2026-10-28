# rust-complex-types-roundtrip

A front-to-back round-trip of a **complex, real-world-shaped message** under
**both** wire representations — XCDR2 (the DDS default) and XCDR1 (classic CDR).

```
optional present:
  [XCDR2] 218 bytes — round-trip OK
  [XCDR1] 200 bytes — round-trip OK
optional absent:
  [XCDR2] 189 bytes — round-trip OK
  [XCDR1] 172 bytes — round-trip OK

all complex-type round-trips pass under both XCDR2 and XCDR1 ✓
```

## The message ([`demo.idl`](demo.idl))

`Sensor` packs together the constructs a hand-rolled codec usually gets wrong:

- a **nested struct** (`Vec3 origin`) and a **`@key`** member,
- a **bounded string** (`string<32> name`),
- an **enum** (`SensorMode` — not `Mode`, which would collide with the member
  `mode`: IDL has one namespace per scope and compares identifiers without
  regard to case, OMG IDL 4.2 §7.2.3),
- a **sequence of discriminated unions** (`sequence<Reading>`, mixing all three
  union arms — `Pose(Vec3)`, `Scalar(double)`, `Raw(octet)`),
- a **map** with a struct value (`map<unsigned long, Vec3> calibration`),
- an **`@optional`** member (`note`) — exercised both present and absent.

`Sensor`, `Vec3` and `Reading` are `@appendable`, so the two representations
differ structurally (not just in alignment):

| | `@appendable` aggregate | 8-byte primitive | here |
|---|---|---|---|
| **XCDR2** | DHEADER-delimited | capped at 4-byte align | 218 / 189 bytes |
| **XCDR1** (classic CDR) | **no** DHEADER | natural 8-byte align | 200 / 172 bytes |

This example asserts every decoded field against its input in both
representations. It is an in-process Rust round-trip, not an independent
cross-vendor or cross-language byte comparison. See
[`idl-conformance/`](../idl-conformance) for the separate language consumers.
The output above was measured against the internal RC7 preparation sources on
2026-09-12, not a published RC7 release.

## How it works

- `Sensor` is generated from `demo.idl` at build time by
  [`zerodds-build`](../../crates/zerodds-build) (see [`build.rs`](build.rs)) —
  the same composition pipeline the `zerodds-idlc` CLI runs, as a library call.
  `dds_type_impls(true)` writes the full `DdsType` impl beside the codec, which
  provides `encode`/`decode` (XCDR2) and `encode_xcdr1`/`decode_xcdr1` (classic
  CDR), each with the correct per-representation framing.
- The generated type also carries *inherent* `encode`/`decode` over a
  `zerodds_wire::Writer`/`Reader`. `src/main.rs` therefore spells the trait
  calls as `<Sensor as DdsType>::encode(…)`; an unqualified `s.encode(&mut b)`
  resolves to the inherent one and does not compile.

## Run

```sh
cargo run
```

Pure in-process encode/decode — no middleware, runs on any platform. Requires a
checked-out [zero-dds](https://github.com/zero-objects/zero-dds) source tree next
to this repo (the build deps reach into `../../crates`).

## Reference

- Website: [complex type round-trip](https://zerodds.org/docs/examples/rust-complex-types-roundtrip.html).
- This example in the repository: [zero-dds-snippets/rust-complex-types-roundtrip](https://github.com/zero-objects/zero-dds-snippets/tree/main/rust-complex-types-roundtrip)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
