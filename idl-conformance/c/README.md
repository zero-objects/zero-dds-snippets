# IDL conformance — C (Foundation profile)

A standalone, runnable example that performs a **real DCPS pub→sub loopback**
over one ZeroDDS participant, using a **typed IDL topic** generated from a
conformance fixture by `zerodds-idlc`. It publishes one fully-populated sample
and takes it back, asserting every field survived the XCDR2 wire — this is not
an in-memory encode/decode check, it goes through DCPS writer→reader matching.

The type is generated from
`tools/idlc/tests/conformance/fixtures/10_keys.idl`:

```idl
module conf {
  struct Keyed {
    @key long      id;
    @key string<16> region;
    double         value;
  };
};
```

## IDL features exercised end-to-end (asserted over the wire)

- **flat struct** as a DCPS topic type
- **`@key` members** — a composite key (`long id` + `string<16> region`); the
  writer/reader are created `is_keyed=1` (WithKey entityKind 0x02) and the
  generated `conf_Keyed_key_hash` builds the 16-byte key hash
- **bounded string** member (`string<16>`, heap-owned `char*`)
- **`long` (int32)** and **`double`** primitive members
- **XCDR2 DHEADER framing** + a per-type **XTypes 1.3 TypeObject** (emitted in
  the generated header)

The published sample is
`{ id=4242, region="EU-CENTRAL-1", value=3.14159265358979 }`; the recovered
sample is compared field-by-field and must match exactly.

## Build & run

First build the C-API shared library and (if not already present) the compiler,
from the repo root:

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
../../../target/debug/zerodds-idlc generate \
    ../../../tools/idlc/tests/conformance/fixtures/10_keys.idl --c -o . </dev/null

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
published: id=4242 region="EU-CENTRAL-1" value=3.14159265358979
recovered: id=4242 region="EU-CENTRAL-1" value=3.14159265358979
ROUNDTRIP OK: recovered sample equals published sample
```

(The IDL compiler reads stdin, so the `generate` invocation always redirects
`</dev/null` — otherwise it blocks waiting for input.)

## Known limitations

### C "Foundation scope" — only flat primitive/bounded-string structs

The `--c` backend is a deliberately **narrow Foundation profile** (tracked as
"Bug C"). It accepts a flat struct of directly-encodable members
(integer/float primitives + bounded `string`) and **rejects** everything else
with `unsupported IDL construct '… (C5.1-a Foundation scope)'`. Verified against
the conformance fixtures with `zerodds-idlc generate <f>.idl --c`:

| Feature                          | Fixture              | C backend result |
|----------------------------------|----------------------|------------------|
| enum                             | `03_enums.idl`       | rejected (`non-struct type`) |
| nested struct                    | `05_nested_structs`  | rejected (`non-primitive type in field`) |
| typedef                          | `06_typedefs.idl`    | rejected (`non-struct type`) |
| sequence                         | `07_sequences.idl`   | rejected (`non-primitive type in field`) |
| array                            | `08_arrays.idl`      | rejected (`non-primitive type in field`) |
| union                            | `09_unions.idl`      | rejected (`non-struct type`) |
| map                              | `13_maps.idl`        | rejected (`non-primitive type in field`) |
| module nesting w/ nested fields  | `16_modules.idl`     | rejected (`non-primitive type in field`) |
| **combo (all of the above)**     | `20_mixed_combo.idl` | rejected (`non-struct type`; also warns the inline-map TypeObject is skipped) |

Because of this, the **combined `20_mixed_combo.idl` type cannot be built
through the C stack at all**, and none of the individual enum / sequence /
union / map / nested / typedef / array feature types round-trip in C. The
example therefore uses the largest C-supported type — the keyed flat struct in
`10_keys.idl` — for the real DCPS roundtrip.

### `@optional` is accepted but NOT modeled (silent flattening)

`11_optional.idl` is the one non-flat fixture the C backend does *not* reject:
it emits code, but it **drops the `@optional` semantics**. For

```idl
struct Optionals { long required; @optional long maybe; @optional string note; };
```

the generated `conf_Optionals_t` is `{ int32_t required; int32_t maybe;
char* note; }` and the encoder writes `maybe`/`note` unconditionally — there is
no presence/absence flag (XTypes §7.2.2.4.4.5 member-is-present member header).
Optionals are therefore **not** faithfully representable, so this example does
not claim `@optional` support; it is treated as out of scope for a correct
roundtrip.

### Header packaging — generated C header self-conflicts with `zerodds.h`

The generated `10_keys.h` includes **both** the cbindgen `zerodds.h` and the
hand-written `zerodds_xcdr2.h`. Those two headers re-declare the same typed FFI
prototypes (`zerodds_writer_write_typed`, `zerodds_reader_take_typed`,
`zerodds_topic_create_typed`, `zerodds_xcdr2_encode/decode`) using
structurally-identical-but-differently-named struct pointers
(`zerodds_typesupport_t*` vs `struct zerodds_ZeroDdsTypeSupport*`, and
`struct ZeroDdsTopic*` vs `struct zerodds_ZeroDdsTopic*`), which a C compiler
rejects as a *conflicting redeclaration*. So a program that pulls in the
generated header alongside the full `zerodds.h` will not compile as-is.

Worked around **only inside this example's own source** (no repo headers
modified): `roundtrip.c` pre-defines the `zerodds.h` include guard (`ZERODDS_H`)
so the generated header's `#include "zerodds.h"` is suppressed — leaving the
typed prototypes + inline XCDR2 helpers from `zerodds_xcdr2.h` — and then
forward-declares by hand the handful of DCPS entity functions it uses
(`zerodds_runtime_create`, `zerodds_writer_create_kind`,
`zerodds_reader_create_kind`, the `wait_for_matched` / `destroy` calls, and
`zerodds_version`). A proper fix belongs in the C-API header packaging (single
canonical struct/typedef name shared by both headers, or guard the duplicate
prototype block).
```

## Files

- `roundtrip.c` — the loopback pub→sub program (hand-written)
- `10_keys.h` — generated by `zerodds-idlc … --c` from `10_keys.idl`
- `Makefile` — `make run` builds + runs; `make gen` refreshes the header
