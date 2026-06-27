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

Live, best-effort readers, integrity-checked payloads, native RTPS 2.5:

| vendor | → ZeroDDS (f / a / m) | ZeroDDS → (f / a / m) |
|---|:--:|:--:|
| **CycloneDDS** | ✅ / ✅ / ✅ | ✅ / ✅ / ✅ |
| **Fast DDS** | ✅ / ✅ / ✅ | ✅ / ✅ / ✅ |
| **OpenDDS** | ✅ / ✅ / ✅ | ✅ / ✅ / ✅ |
| **RTI Connext** | ✅ / ✅ / ✅ | ✅ / ✅ / ✅ |

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

Part of **[ZeroDDS](https://zerodds.de)**.
