# Cross-vendor rich-type interop — extensibility × XCDR × endianness

A richer companion to [`../cross-vendor-shapes`](../cross-vendor-shapes): instead of
the flat `ShapeType`, this exercises a keyed struct with an 8-byte primitive and a
variable-length sequence, in **all three XTypes extensibilities**, against four
other DDS stacks — **CycloneDDS, eProsima Fast DDS, OpenDDS and RTI Connext** — in
**both directions**.

```idl
@final      struct RichF { @key unsigned long id; double value; sequence<octet> blob; };
@appendable struct RichA { @key unsigned long id; double value; sequence<octet> blob; };
@mutable    struct RichM { @key unsigned long id; double value; sequence<octet> blob; };
```

The point of the three variants is that each maps to a **different wire form**, and
the four stacks do not agree on which XCDR version to use for them. ZeroDDS speaks
all of them — that interchangeability is what this example proves.

## What each writer puts on the wire (measured encapsulation id)

The serialized-payload **encapsulation id** is a tuple of *(XCDR version ×
extensibility × endianness)*. Captured live, the stacks fall into three dialects:

| writer (little-endian) | `@final` | `@appendable` | `@mutable` |
|---|:--:|:--:|:--:|
| **ZeroDDS** / **OpenDDS** | `0x0007` CDR2 (XCDR2) | `0x0009` D_CDR2 (XCDR2) | `0x000b` PL_CDR2 (XCDR2) |
| **CycloneDDS** (hybrid) | `0x0001` CDR (**XCDR1**) | `0x0009` D_CDR2 (XCDR2) | `0x000b` PL_CDR2 (XCDR2) |
| **Fast DDS** / **RTI Connext** | `0x0001` CDR (**XCDR1**) | `0x0001` CDR (**XCDR1**) | `0x0003` PL_CDR (**XCDR1**) |

`@final`/`@appendable` collapse to plain CDR under XCDR1 (no DHEADER). Fast DDS and
RTI share `0x0003` for `@mutable` yet differ one level down — Fast DDS uses the
short parameter id, RTI uses the **PID_EXTENDED** long form with the
MUST_UNDERSTAND flag (`0x7F01`).

## Result — every cell delivers correct data

Live, integrity-checked payloads, native RTPS 2.5, one Linux host; each cell a
domain of its own. Measured by `ci/run-examples.sh` (rows
`cross-vendor-rich/{cyclone,fastdds,opendds,opensplice,rti}`). OpenSplice has no
extensibility kinds, so only its `@final` cell exists (`–`); see
[`opensplice/`](opensplice/):

| vendor | → ZeroDDS (f / a / m) | ZeroDDS → (f / a / m) |
|---|:--:|:--:|
| **CycloneDDS** | ✅ / ✅ / ✅ | ✅ / ✅ / ✅ |
| **Fast DDS** | ✅ / ✅ / ✅ | ✅ / ✅ / ✅ |
| **OpenDDS** | ✅ / ✅ / ✅ | ✅ / ✅ / ✅ |
| **RTI Connext** | ✅ / ✅ / ✅ | ✅ / ✅ / ✅ |
| **OpenSplice CE 6.9** | ✅ / – / – | ✅ / – / – |

Three legs need a setting on one side, and each `run.sh` makes it:

- **Fast DDS** writes and reads its own CDR dialect: `@mutable` members under
  bare parameter ids with true lengths, no must-understand flag on a key
  member. `fastdds/run.sh` builds the ZeroDDS side with
  `ZERODDS_RICH_DIALECT=fastdds`, into a target directory of its own. Its
  writer also announces `PID_TYPE_MAX_SIZE_SERIALIZED` = 24 for `RichF` and
  sends samples of 76 octets; a ZeroDDS reader holds a writer to that bound
  (DDSI-RTPS 2.5 Table 8.72), so `fastdds/run.sh` sets
  `ZERODDS_READER_HONOR_DATA_MAX_SIZE_SERIALIZED=false`.
- **OpenDDS 3.34** sets IS_MUST_UNDERSTAND on a key member only when
  `@must_understand` is written, reads a remote reader's ALLOW_TYPE_COERCION
  as DISALLOW and then matches identical type identifiers only;
  [`opendds/Rich.idl`](opendds/Rich.idl) writes the flag out. Its DataReader
  accepts XCDR2 only and drops an XCDR1 sample, so `opendds/run.sh` starts the
  ZeroDDS writer with `ZERODDS_DATA_REPR_OFFER=XCDR2`. OpenDDS also declares
  encapsulation padding it does not append; ZeroDDS counts padding only where
  the payload length allows it.
- **RTI Connext** bounds `sequence<octet>` at 100 unless `rtiddsgen` runs with
  `-unboundedSupport`, which `rti/run.sh` passes. It declares a PID_EXTENDED
  member's length rounded up to four, which the specification's dialect reads
  as a framing error, so `rti/run.sh` builds the ZeroDDS side with
  `ZERODDS_RICH_DIALECT=rti`.

ZeroDDS reads and writes every wire form above — XCDR1 plain CDR, XCDR1 PL_CDR
(including RTI's PID_EXTENDED), XCDR2 CDR2 / D_CDR2 / PL_CDR2.

### Endianness — the third axis

Every stack on a little-endian host *emits* little-endian. To exercise the
big-endian half, the ZeroDDS writer can be told to emit big-endian
(`ZERODDS_WIRE_BIG_ENDIAN=1` → encapsulation `0x0006` / `0x0008` / `0x000a`). All
four stacks decode ZeroDDS's big-endian samples across all three extensibilities —
a DDS reader picks the byte order per sample from the encapsulation header.

## Layout

| dir | stack | language |
|---|---|---|
| [`zerodds/`](zerodds) | ZeroDDS | Rust (generated types) |
| [`cyclone/`](cyclone) | CycloneDDS | Python |
| [`fastdds/`](fastdds) | eProsima Fast DDS | C++ |
| [`opendds/`](opendds) | OpenDDS | C++ |
| [`rti/`](rti) | RTI Connext | C++11 |

Each vendor dir has a README with its build + run steps. The `id`/`value`/`blob`
fields and the topic names `RichF` / `RichA` / `RichM` are identical across all
stacks; the `blob` is filled with `k % 251` so the subscriber can verify byte
integrity end to end.

## See also

[**roundtrip-perf.md**](roundtrip-perf.md) — full ping/pong round-trip **latency**
across all five stacks in four conditions (plain/rich × clear/secured), the
event-driven same-host SHM carrier, and the per-vendor requirements (RTI XCDR2
compliance mask, OpenDDS governance, Fast DDS RSA certs) that make every
ZeroDDS↔vendor cell deliver data.

## Reference

- No page on the website explains this example yet; this README is the whole of it.
- This example in the repository: [zero-dds-snippets/cross-vendor-rich](https://github.com/zero-objects/zero-dds-snippets/tree/main/cross-vendor-rich)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
