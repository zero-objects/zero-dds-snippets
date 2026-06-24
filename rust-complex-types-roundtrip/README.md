# rust-complex-types-roundtrip

A front-to-back round-trip of a **complex, real-world-shaped message** under
**both** wire representations — XCDR2 (the DDS default) and XCDR1 (classic CDR).

```
optional present:
  [XCDR2] 230 bytes — round-trip OK
  [XCDR1] 198 bytes — round-trip OK
optional absent:
  [XCDR2] 201 bytes — round-trip OK
  [XCDR1] 169 bytes — round-trip OK

all complex-type round-trips pass under both XCDR2 and XCDR1 ✓
```

## The message ([`demo.idl`](demo.idl))

`Sensor` packs together the constructs a hand-rolled codec usually gets wrong:

- a **nested struct** (`Vec3 origin`) and a **`@key`** member,
- a **bounded string** (`string<32> name`),
- an **enum** (`Mode`),
- a **sequence of discriminated unions** (`sequence<Reading>`, mixing all three
  union arms — `Pose(Vec3)`, `Scalar(double)`, `Raw(octet)`),
- a **map** with a struct value (`map<unsigned long, Vec3> calibration`),
- an **`@optional`** member (`note`) — exercised both present and absent.

`Sensor`, `Vec3` and `Reading` are `@appendable`, so the two representations
differ structurally (not just in alignment):

| | `@appendable` aggregate | 8-byte primitive | here |
|---|---|---|---|
| **XCDR2** | DHEADER-delimited | capped at 4-byte align | 230 / 201 bytes |
| **XCDR1** (classic CDR) | **no** DHEADER | natural 8-byte align | 198 / 169 bytes |

Every value round-trips cleanly in each representation. ZeroDDS produces
byte-identical XCDR2 *and* XCDR1 wire for this type across **all seven** language
bindings (Rust, Python, C#, TypeScript, Java, C++, C) — the same corpus is
checked end-to-end in [`idl-conformance/`](../idl-conformance) against the other
PSMs and against Cyclone DDS / FastDDS goldens.

## How it works

- `Sensor` is generated from `demo.idl` at build time by the ZeroDDS IDL→Rust
  backend (see [`build.rs`](build.rs)) — the same `idlc --rust` code path. The
  full `DdsType` impl provides `encode`/`decode` (XCDR2) and
  `encode_xcdr1`/`decode_xcdr1` (classic CDR), each with the correct
  per-representation framing.

## Run

```sh
cargo run
```

Pure in-process encode/decode — no middleware, runs on any platform. Requires a
checked-out [zero-dds](https://github.com/zero-objects/zero-dds) source tree next
to this repo (the build deps reach into `../../crates`).

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
