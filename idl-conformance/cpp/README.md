# idl-conformance / cpp

A standalone, runnable **real-DCPS** conformance example for the ZeroDDS C++
binding. It publishes one fully-populated sample of a multi-feature IDL type
over a real ZeroDDS DCPS participant (domain 0, loopback) and reads it back,
asserting every recovered field equals what was published — proving the data
survived the XCDR2 **wire**, not just an in-memory encode/decode.

The type is `combo::Telemetry` from the conformance fixture
[`tools/idlc/tests/conformance/fixtures/20_mixed_combo.idl`](../../../tools/idlc/tests/conformance/fixtures/20_mixed_combo.idl),
generated with `zerodds-idlc ... --cpp` into [`gen/`](gen/).

## IDL features exercised (all round-trip over DCPS)

One keyed topic type combines:

| Feature                       | IDL                         | Verified field(s)          |
|-------------------------------|-----------------------------|----------------------------|
| `@key` integer                | `@key long unitId`          | `unitId`                   |
| `@key` bounded string         | `@key string<32> region`    | `region`                   |
| enum                          | `Mode mode`                 | `mode`                     |
| typedef → primitive           | `CurrentInAmpsType battery` | `batteryCurrent`           |
| sequence of nested struct     | `sequence<Sample> history`  | each `history[i].{seq,value}` |
| nested struct                 | `struct Sample`             | (inside the sequence)      |
| map<string,long>              | `map<string,long> counters` | `counters[ok/err/retry]`   |
| `@optional`                   | `@optional double calibration` | presence + value        |
| fixed array                   | `long window[4]`            | all 4 elements             |
| `@appendable` extensibility   | `struct Telemetry`          | DHEADER framing            |

Each of these is asserted equal after the DCPS roundtrip; the program prints a
`[PASS]`/`[FAIL]` line per field and exits non-zero if any field differs.

## Build & run

The C-API cdylib `libzerodds.dylib` must exist under `target/release` (it is
already built in this checkout). To (re)build it from the ZeroDDS checkout:

```sh
cargo build -p zerodds-c-api --release
```

The generated header in `gen/` is checked in. To regenerate it from the IDL:

```sh
/Users/sandrakessler/projects/zerodds/target/debug/zerodds-idlc generate \
    /Users/sandrakessler/projects/zerodds/tools/idlc/tests/conformance/fixtures/20_mixed_combo.idl \
    --cpp -o gen </dev/null
```

Compile and run (from this directory):

```sh
clang++ -std=c++17 combo_roundtrip.cpp \
  -I . \
  -I /Users/sandrakessler/projects/zerodds/crates/cpp/include \
  -I /Users/sandrakessler/projects/zerodds/crates/zerodds-c-api/include \
  -L /Users/sandrakessler/projects/zerodds/target/release -lzerodds \
  -o combo_roundtrip

DYLD_LIBRARY_PATH=/Users/sandrakessler/projects/zerodds/target/release ./combo_roundtrip
```

Or just `sh build.sh` then run the printed command.

Expected output ends with:

```
ROUNDTRIP OK: all wire-supported combo features survived DCPS.
```

## Known limitations

1. **Union member dropped on the wire.** `combo::Telemetry` has a union member
   `Reading reading` (`union Reading switch (Mode)`). The current cpp codegen
   generates the `Reading` class but **silently skips it in encode and decode**,
   emitting:

   ```cpp
   // xcdr2: member 'reading' not supported (nested/enum/map/fixed; skip)
   ```

   The skip is symmetric, so the rest of the sample still round-trips, but the
   union value itself does **not** survive the wire. The example populates
   `reading` but excludes it from the equality assertions and prints a note.
   This is a real codegen gap (cpp-codegen layer): union members are not
   serialized.

2. **Byte-oriented write path drops the XCDR representation tag.** When you
   publish via the byte-oriented C-API (`zerodds_writer_write`) and read back
   with `zerodds_reader_take`, the `out_repr` out-param comes back `0` (XCDR1)
   even though the payload was encoded as XCDR2 — the wire bytes are
   byte-identical, only the representation tag is lost. `zerodds::TypedReader<T>`
   trusts that tag and would decode with the wrong alignment (XCDR1 `max_align=8`
   vs XCDR2 `max_align=4`), causing a `buffer underrun`. This example therefore
   uses the byte-oriented `zerodds::Writer` / `zerodds::Reader` and decodes
   explicitly with the same `XcdrVersion::Xcdr2` it encoded with. This is a
   c-api/runtime layer gap, not a codegen gap; it is worked around here without
   modifying any binding source.

## Reference

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS
implementation. Companion to the C++ binding examples
[`cpp-typed`](../../cpp-typed/) and [`cpp-hello`](../../cpp-hello/).
