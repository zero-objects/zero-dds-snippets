# rust-xcdr1-representation

Picking the **data representation** for a typed sample — **XCDR2** (the DDS /
ZeroDDS default) vs **XCDR1** (classic CDR) — and seeing the difference on the
wire.

```
sample: Reading { seq: 7, value: 2.5, label: "ok" }

XCDR2 (19 bytes): 07 00 00 00 00 00 00 00 00 00 04 40 03 00 00 00 6f 6b 00
XCDR1 (23 bytes): 07 00 00 00 00 00 00 00 00 00 00 00 00 00 04 40 03 00 00 00 6f 6b 00
                              └── XCDR1 pads `value` to an 8-byte boundary ──┘
```

`Reading` carries a 4-byte `seq` immediately followed by an 8-byte `double`
`value`. That is exactly where the two representations diverge:

| | 8-byte primitive alignment | `value` lands at | size here |
|---|---|---|---|
| **XCDR2** (OMG XTypes 1.3) | capped at **4** | offset 4 | 19 bytes |
| **XCDR1** (classic CDR / `DDS_CDR`) | natural **8** | offset 8 (4 pad bytes) | 23 bytes |

XCDR2 is the DDS default and what ZeroDDS negotiates with peers out of the box.
XCDR1 is the pre-XTypes wire that many **legacy** and **FastDDS-classic** peers
still speak — ZeroDDS implements the full XCDR1 codec in **every** language
binding (Rust, Python, C#, TypeScript, Java, C++, C), byte-identical across all
of them, so it can interoperate either way.

## How it works

- `Reading` is generated from [`demo.idl`](demo.idl) at build time by
  `zerodds-build` (see [`build.rs`](build.rs)), which runs the same
  composition pipeline the `zerodds-idlc` CLI runs. `dds_type_impls(true)`
  is what emits the second codec pair, `encode_xcdr1`/`decode_xcdr1`,
  beside `encode`/`decode`.
- The representation is the alignment cap the `zerodds_wire::Writer` and
  `Reader` were built with, XTypes 1.3 §7.4.1: `with_max_align(…, 4)` for
  XCDR2, `with_max_align(…, 8)` for XCDR1.
- A reader and writer must agree on the representation (a mismatch would
  mis-align `value`). In DDS this is negotiated via the `DataRepresentation`
  QoS and signalled by the RTPS encapsulation id (`CDR_LE` 0x0001 for XCDR1,
  `D_CDR2_LE` 0x0009 for XCDR2).

`Reading` is `@final`, which isolates the *pure alignment* difference (no
DHEADER either way). `@appendable` adds a DHEADER under XCDR2 only, and
`@mutable` switches between PL_CDR2 (EMHEADER) and PL_CDR1 (a PID parameter
list) — those are handled by the full `DdsType::encode_xcdr1` and are exercised
in the cross-PSM conformance corpus under `idl-conformance/`.

## Run

```sh
cargo run
```

No external middleware needed — this is a pure in-process encode/decode demo and
runs on any platform. Requires a checked-out
[zero-dds](https://github.com/zero-objects/zero-dds) source tree next to this
repo (the build deps reach into `../../crates`).

## Reference

- No page on the website explains this example yet; this README is the whole of it.
- This example in the repository: [zero-dds-snippets/rust-xcdr1-representation](https://github.com/zero-objects/zero-dds-snippets/tree/main/rust-xcdr1-representation)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
