# idl-conformance / python

A **standalone, runnable** IDL-conformance example for the ZeroDDS **Python**
language binding (PSM). It publishes fully-populated typed IDL samples over a
**real DCPS DomainParticipant + publisher/subscriber loopback** (not just
in-memory encode/decode) and reads them back, asserting the data survived the
wire.

The types are generated from IDL by the pre-built compiler
(`target/debug/zerodds-idlc ... --python`), except where a real codegen bug
forced a hand-written stand-in (clearly marked — see *Known limitations*).

## IDL features exercised (end-to-end over the wire)

Topic 1 — **`Telemetry`** (a combined / cross-feature type, generated from
`combo_telemetry.idl`):

| Feature                     | IDL                                  |
|-----------------------------|--------------------------------------|
| `enum`                      | `enum Mode {...}` used as a member   |
| nested `struct`             | `Sample` inside `Telemetry`          |
| sequence-of-struct          | `sequence<Sample> history`           |
| bounded string              | `string region` (`@key`)             |
| `map`                       | `map<string,long> counters`          |
| array                       | `long window[4]`                     |
| `typedef`                   | `CurrentInAmpsType` -> `double`      |
| `@key`                      | `@key long unitId; @key string region` |
| `@appendable` extensibility | struct-level annotation              |

Topic 2 — **`Reading`** (generated from `reading_union.idl`): an IDL **`union`**
with an **enum discriminator**, an explicit case, and a **`default`** case.
Both the active case and the default case are round-tripped.

Topic 3 — **`Optionals`**: IDL **`@optional`** members, round-tripped in both
the *present* and *absent* (None present-flag) states.

## Files

| File                        | Origin                                                      |
|-----------------------------|-------------------------------------------------------------|
| `combo_telemetry.idl`       | source IDL for topic 1                                      |
| `combo_telemetry.py`        | **generated** by `zerodds-idlc --python`                    |
| `reading_union.idl`         | source IDL for topic 2                                      |
| `reading_union.py`          | **generated** by `zerodds-idlc --python`                    |
| `optionals_handwritten.py`  | **hand-written** (codegen drops `@optional`, see below)     |
| `conformance_roundtrip.py`  | the runnable pub->sub loopback program                      |

## Build & run

The native extension (`zerodds._core.abi3.so`) is already built in the
checkout, so there is **no maturin step**. Just point `PYTHONPATH` at the
Python runtime package and run:

```sh
cd zerodds-examples/idl-conformance/python
PYTHONPATH=/Users/sandrakessler/projects/zerodds/crates/py/python \
  python3 conformance_roundtrip.py
```

Expected tail of the output:

```
================================================================
ALL ROUNDTRIPS PASSED
================================================================
```

### Regenerating the types (optional)

```sh
ZIDLC=/Users/sandrakessler/projects/zerodds/target/debug/zerodds-idlc
$ZIDLC generate combo_telemetry.idl --python -o . </dev/null
$ZIDLC generate reading_union.idl   --python -o . </dev/null
```

(The CLI reads stdin; always pass `</dev/null` or it hangs. The
`map (inline IDL map)` TypeObject warning is benign — the Python wire codec
handles the map; only the optional XTypes TypeObject blob is skipped.)

## Known limitations

These were excluded from the combined `Telemetry` type because of **real**
codegen/runtime bugs in the current Python stack — they are documented here
rather than faked.

1. **Module-wrapped types do not work at all (Bug Q-cluster).**
   The canonical fixture `20_mixed_combo.idl` wraps everything in
   `module combo {...}`. The Python backend flattens the classes to
   `combo_Telemetry`, `combo_Sample`, etc., but the field annotations and the
   `idl_union(discriminator=...)` call still reference the **bare** names
   (`Telemetry`, `Sample`, `Mode`), which are never defined -> the generated
   module raises `NameError: name 'Mode' is not defined` on import.
   *Work-around in this example:* the IDL was rewritten **top-level (no
   `module`)** so the generated names resolve. All ZeroDDS conformance
   fixtures are module-wrapped, so none of them can be used directly.

2. **`union` as a struct member is rejected at runtime.**
   The Python backend emits the union member of `Telemetry` as a plain field
   typed by the `idl_union` facade, but
   `zerodds.idl._kind_from_annotation` has no branch for that facade and
   raises
   `TypeError: @idl_struct: field type <... _UnionFacade> not supported`.
   It also emits `reading: Reading` plus a `Reading('activeRate', 9.81)`-style
   constructor call, but the facade is constructed via `Reading.make(disc,
   value)`, not a positional constructor.
   *Work-around:* the union is exercised as a **standalone top-level union
   topic** (topic 2), where the runtime `idl_union` API round-trips correctly.

3. **`@optional` is silently dropped by the codegen (data-fidelity bug).**
   For `@optional long maybe;` the Python backend emits `maybe: Int32`
   (a required field, no present-flag) instead of `Optional[Int32]`.
   The runtime (`zerodds.idl.Optional`) supports the optional present-flag
   wire form perfectly, so `optionals_handwritten.py` contains the type a
   correct backend *should* emit, and topic 3 round-trips it (present + absent).

4. **Fixed-size array brand is lost (`long window[4]` -> `List[Int32]`).**
   The backend emits arrays as `List[T]` (an unbounded sequence brand) rather
   than `Array[T, N]`. The 4 values still round-trip (a 4-element sequence is
   wire-compatible with the data we send), but the **fixed bound** is not
   enforced by the generated type. Topic 1 therefore exercises the *array
   data* but not the bound check.

The combined fixed-array bound check (limitation 4) and a union-inside-struct
(limitation 2) are the only features the combined topic does not fully honour;
everything else in the table above round-trips faithfully.

## Reference

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS
implementation. Companion to the **[Python bindings](https://zerodds.de/bindings/python/)**
documentation.
