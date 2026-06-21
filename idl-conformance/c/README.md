# IDL conformance — C (Foundation profile)

A standalone, runnable example that performs a **real DCPS pub→sub loopback**
over one ZeroDDS participant, using a **typed IDL topic** generated from an IDL
fixture by `zerodds-idlc`. It publishes one fully-populated sample and takes it
back, asserting every field survived the XCDR2 wire — this is not an in-memory
encode/decode check, it goes through DCPS writer→reader matching.

The type is generated from this example's local fixture `sensor.idl`:

```idl
module conf {
  enum Status { OFFLINE, NOMINAL, DEGRADED, FAULT };
  typedef double  Celsius;
  typedef Celsius TemperatureType;          // alias chain → double
  struct GeoPoint { double lat; double lon; };
  struct SensorReading {
    @key long        sensor_id;             // long (int32) key
    @key string<16>  site;                  // bounded-string key
    Status           status;                // enum member
    TemperatureType  temperature;           // typedef-to-double
    GeoPoint         location;              // nested struct member
    long             samples[4];            // fixed array member
    @optional double calibration;           // @optional primitive
    @optional string note;                  // @optional string
  };
};
```

## IDL features exercised end-to-end (asserted over the wire)

- **flat-but-rich keyed struct** as a DCPS topic type
- **`@key` members** — a composite key (`long sensor_id` + `string<16> site`);
  the writer/reader are created `is_keyed=1` (WithKey entityKind 0x02) and the
  generated `conf_SensorReading_key_hash` builds the 16-byte key hash from both
  key members
- **enum member** — `conf::Status` (encoded as int32; `conf_Status_DEGRADED`)
- **typedef-to-primitive + alias chain** — `TemperatureType → Celsius → double`
  collapses to a plain `double temperature` member
- **nested struct member** — `conf::GeoPoint location` (`{ double lat; double lon; }`)
- **fixed-size array member** — `long samples[4]` (encoded element-by-element)
- **`@optional` members with presence flags** — `calibration` and `note` each
  get a `_present` companion byte (XTypes-style member-is-present marker); both
  are published *present* and the presence flag + value are asserted on recovery
- **bounded string** member (`string<16>`, heap-owned `char*`)
- **`long` (int32)** and **`double`** primitive members
- **module nesting** (`module conf { ... }`)
- **XCDR2 DHEADER framing** + per-type **XTypes 1.3 TypeObjects** (emitted in
  the generated header)

The published sample sets every member (both optionals present); the recovered
sample is compared field-by-field — including the optional presence flags — and
must match exactly.

## Build & run

First build the C-API shared library, from the repo root:

```sh
cargo build -p zerodds-c-api --release
# the compiler binary used here is the pre-built target/debug/zerodds-idlc
```

Then, from this directory:

```sh
make run
```

`make run` compiles `roundtrip.c` (linking `libzerodds`) and runs it with the
right dynamic-library path for your platform (`DYLD_LIBRARY_PATH` on macOS,
`LD_LIBRARY_PATH` on Linux).

Equivalent manual commands (macOS shown; use `LD_LIBRARY_PATH` on Linux):

```sh
# (optional) regenerate the typed header from the fixture:
../../../target/debug/zerodds-idlc generate sensor.idl --c -o . </dev/null

gcc -std=c11 -Wall roundtrip.c \
    -I . \
    -I ../../../crates/zerodds-c-api/include \
    -L ../../../target/release -lzerodds \
    -o roundtrip

DYLD_LIBRARY_PATH=../../../target/release ./roundtrip
```

Expected output:

```
zerodds version: 1.0.0-rc.3.1
published: id=4242 site="EU-CENTRAL-1" status=2 temp=36.625 loc=(50.110924,8.682127) samples=[11,22,33,44] calib=0.998001(present=1) note="recalibrate"(present=1)
recovered: id=4242 site="EU-CENTRAL-1" status=2 temp=36.625 loc=(50.110924,8.682127) samples=[11,22,33,44] calib=0.998001(present=1) note="recalibrate"(present=1)
ROUNDTRIP OK: recovered sample equals published sample (...)
```

(The IDL compiler reads stdin, so the `generate` invocation always redirects
`</dev/null` — otherwise it blocks waiting for input.)

## Known limitations

The `--c` backend is a **Foundation profile**: it now generates and round-trips
flat-but-rich keyed structs (enum, typedef-to-primitive incl. alias chains,
nested struct, fixed array, `@optional` with presence flags, module nesting —
all exercised above). The earlier flat-primitive-only scope, the silent
`@optional` flattening, and the generated-header-vs-`zerodds.h` conflict are all
**resolved** and no longer apply.

The following constructs are **still rejected** by the C backend (genuine
remaining gaps; verified with
`zerodds-idlc generate <f>.idl --c </dev/null`):

| Feature                              | C backend result |
|--------------------------------------|------------------|
| `union`                              | rejected — `unsupported IDL construct 'union (C backend)'` |
| `map`                                | rejected — inline-map TypeObject skipped + `'non-primitive type in field'` |
| `wstring`                            | rejected — `unsupported IDL construct 'wstring'` |
| `fixed<M,N>` (fixed-point decimal)   | rejected — `unsupported IDL construct 'non-primitive type in field'` (TypeObject also skipped) |
| array bound by named constant (`v[N]`) | rejected — `unsupported IDL construct 'non-literal array bound'` |
| `sequence<T>` of **non-primitive** T | rejected — `unsupported IDL construct 'non-primitive sequence element'` |
| `typedef` to an **aggregate** (sequence/array/struct alias used as a member) | not modeled as the aliased aggregate in C |

Notes on scope:

- A `sequence<T>` of a **primitive** element *does* now emit C encode/decode
  code, but this example deliberately keeps **all** `sequence` members out of
  the C profile (resizable heap-owned collections are intentionally out of the
  C Foundation scope here), so it is listed as a remaining gap rather than
  exercised.
- Because of the gaps above, the combined `20_mixed_combo.idl` type (which
  pulls in `union` + `map` + non-primitive `sequence`) still cannot be built
  through the C stack. The example therefore uses the largest C-supported type —
  the keyed `SensorReading` struct in `sensor.idl` — for the real DCPS
  roundtrip.

## Files

- `roundtrip.c` — the loopback pub→sub program (hand-written)
- `sensor.idl` — the IDL fixture for this example
- `sensor.h` — generated by `zerodds-idlc … --c` from `sensor.idl`
- `Makefile` — `make run` builds + runs; `make gen` refreshes the header
