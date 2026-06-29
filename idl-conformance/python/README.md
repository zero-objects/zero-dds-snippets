# idl-conformance / python

A **standalone, runnable** IDL-conformance example for the ZeroDDS **Python**
language binding (PSM). It publishes a fully-populated typed IDL sample over a
**real DCPS DomainParticipant + publisher/subscriber loopback** (not just
in-memory encode/decode) and reads it back, asserting the data survived the
wire.

The type is generated from the **unmodified canonical conformance fixture**
`20_mixed_combo.idl` by the pre-built compiler
(`target/debug/zerodds-idlc ... --python`) — no hand-written stand-ins, no
hand-edited IDL.

## Single combined topic

The whole example is one keyed topic, `combo::Telemetry`, generated as-is from
the module-wrapped fixture. A single sample exercises every feature below
together (catching cross-feature interactions over a real DCPS roundtrip):

| Feature                       | IDL                                       |
|-------------------------------|-------------------------------------------|
| `enum` member                 | `Mode mode`                               |
| nested `struct`               | `Sample` inside `Telemetry`               |
| sequence-of-struct            | `sequence<Sample> history`                |
| bounded string (`@key`)       | `@key string<32> region`                  |
| **`union` as a struct member**| `Reading reading` (enum discriminator)    |
| `map`                         | `map<string,long> counters`               |
| **`@optional`**               | `@optional double calibration`            |
| **fixed-size array**          | `long window[4]` → `Array[Int32, 4]`      |
| `typedef`                     | `CurrentInAmpsType` → `double`            |
| `@key`                        | `@key long unitId; @key string region`    |
| `@appendable` extensibility   | struct-level annotation                   |

The roundtrip publishes the sample twice on the same topic to cover the
variable cases:

- **`@optional` present** + union **active case** (case 1, `double activeRate`).
- **`@optional` absent** (`None` present-flag) + union **default case**
  (`string<16> faultCode`).

## Files

| File                       | Origin                                         |
|----------------------------|------------------------------------------------|
| `20_mixed_combo.idl`       | the unmodified canonical conformance fixture   |
| `20_mixed_combo.py`        | **generated** by `zerodds-idlc --python`       |
| `conformance_roundtrip.py` | the runnable pub→sub loopback program          |

## Build & run

The native extension (`zerodds._core.abi3.so`) is already built in the
checkout, so there is **no maturin step**. Just point `PYTHONPATH` at the
Python runtime package and run:

```sh
cd zerodds-examples/idl-conformance/python
PYTHONPATH=/path/to/zerodds/crates/py/python \
  python3 conformance_roundtrip.py
```

Expected tail of the output:

```
================================================================
ALL ROUNDTRIPS PASSED
================================================================
```

### Regenerating the type (optional)

```sh
ZIDLC=/path/to/zerodds/target/debug/zerodds-idlc
$ZIDLC generate 20_mixed_combo.idl --python -o . </dev/null
```

(The CLI reads stdin; always pass `</dev/null` or it hangs.)

## Known limitations

Only one genuine gap remains; it does **not** affect the roundtrip:

1. **The XTypes `TypeObject` blob is not emitted for the inline `map` member.**
   Generation prints `warning: TypeObject emission skipped —
   UnsupportedTypeSpec("map (inline IDL map)")`. This is benign for this
   example: the Python **wire codec** encodes/decodes `map<string,long>`
   correctly (the `counters` field round-trips faithfully), and only the
   optional XTypes TypeObject metadata blob — used for remote type discovery /
   assignability — is skipped for the map. The topic still matches and the
   sample still round-trips over the wire.

Everything else in the feature table above round-trips faithfully, in a single
module-wrapped topic generated from the unmodified conformance fixture.

> **Previously-excluded features now fully working** (fixed in the rebuilt
> compiler/runtime, so no longer worked around here):
> module-wrapped scoped-name resolution (`module combo {...}` imports as-is),
> `union` as a struct member, `@optional` present/absent present-flags, and the
> fixed-size array brand (`long window[4]` → `Array[Int32, 4]`, bound enforced).
> Earlier this example had to decompose the fixture into three separate topics
> and hand-write the `@optional` type; that is no longer necessary.

## Reference

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS
implementation. Companion to the **[Python bindings](https://zerodds.de/bindings/python/)**
documentation.
