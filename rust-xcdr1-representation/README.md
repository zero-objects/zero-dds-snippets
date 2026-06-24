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

- `Reading` is generated from [`demo.idl`](demo.idl) at build time by the
  ZeroDDS IDL→Rust backend (see [`build.rs`](build.rs)) — the same `idlc --rust`
  code path, in `cdr_only` mode (just the type + its `CdrEncode`/`CdrDecode`).
- The representation is selected by the `BufferWriter`'s **max alignment**:
  `with_max_alignment(4)` for XCDR2, `with_max_alignment(8)` for XCDR1.
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

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
