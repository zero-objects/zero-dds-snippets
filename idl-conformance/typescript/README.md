# idl-conformance / typescript

A **standalone, runnable** IDL-conformance example for the ZeroDDS TypeScript
(`idl-ts`) PSM. It generates a typed topic from IDL with `zerodds-idlc --ts`,
then publishes one fully-populated sample over a **real DCPS participant +
Publisher/DataWriter** and reads it back through a **Subscriber/DataReader** on
the loopback transport, asserting the recovered sample is deep-equal to what was
published. This is a genuine RTPS pub→sub roundtrip via the `@zerodds/node` FFI
binding (native `libzerodds`), **not** an in-memory encode/decode.

## What IDL features are exercised

The topic type is `combo::Telemetry` in [`idl/telemetry_combo.idl`](idl/telemetry_combo.idl),
the **complete, unmodified** upstream conformance combo
`tools/idlc/tests/conformance/fixtures/20_mixed_combo.idl`. The single keyed
sample combines, end-to-end over the wire:

| Feature                       | IDL member                          | Verified |
|-------------------------------|-------------------------------------|----------|
| `@key` integer                | `@key long unitId`                  | ✅ |
| `@key` + bounded string       | `@key string<32> region`            | ✅ |
| **enum-typed member**         | `Mode mode`                         | ✅ |
| `typedef` (to `double`)       | `CurrentInAmpsType batteryCurrent`  | ✅ |
| nested struct                 | `struct Sample { long; double; }`   | ✅ |
| sequence **of struct**        | `sequence<Sample> history`          | ✅ |
| **union member**              | `Reading reading` (`switch(Mode)`)  | ✅ |
| `map<string, long>`           | `map<string,long> counters`         | ✅ |
| `@optional` (present)         | `@optional double calibration`      | ✅ |
| **fixed-size array member**   | `long window[4]`                    | ✅ |

The roundtrip asserts deep equality of every field — including `Map` contents,
the `@optional` member, the `enum` member, the discriminated `union` branch and
all four `window[4]` elements — after the bytes traverse the DCPS stack. The
enum, union and fixed-array members each get an additional explicit assertion so
a regression in those codecs is unambiguous.

## Build & run

The pre-built compiler and the native binding must already exist in the repo
(`target/debug/zerodds-idlc`, `crates/ts-node/dist/`, and a built
`target/debug/libzerodds.dylib`/`.so`). From **this** directory:

```sh
# 1. install the dev toolchain (tsx + typescript + @types/node)
npm install

# 2. link the @zerodds/node binding (relative symlink into node_modules)
npm run link

# 3. (re)generate the typed module from IDL — already committed under gen/,
#    re-run only if you change the .idl:
../../../target/debug/zerodds-idlc generate \
    idl/telemetry_combo.idl --ts -o gen </dev/null

# 4. typecheck and run the roundtrip
npm run typecheck      # tsc --noEmit
npm run start          # tsx src/roundtrip.ts
```

`@zerodds/cdr` and `@zerodds/types` are resolved through `tsconfig.json`
`paths` (the in-repo `crates/ts-node/dist/cdr` and `crates/idl-ts/src/runtime`),
so no extra packaging is needed for them. Only `@zerodds/node` needs the
`npm run link` symlink because it loads the native library and its bundled
`koffi` FFI runtime.

Expected output:

```
PUBLISHED: {"unitId":4711,"region":"eu-central-1",...}
RECEIVED : {"unitId":4711,"region":"eu-central-1",...}
ROUNDTRIP OK — combo::Telemetry survived the DCPS wire intact.
```

## Known limitations

The example now publishes the **complete** upstream combo type — all ten
features above round-trip over real DCPS. The three `idl-ts` codegen bugs that
previously forced a pruned subset (union member gating off the struct codec,
fixed-array member emitted as a scalar, enum member encoded as its string label)
are **fixed**: the regenerated `gen/telemetry_combo.ts` now writes the union
discriminator + branch, loops `window[4]`, and routes the enum through
`ModeOrdinal` / `ModeFromOrdinal`. The roundtrip run proves it.

Two minor, **non-blocking** cosmetic items remain in the generated module — they
do not affect the DCPS roundtrip and the codec is correct regardless:

1. **`map` member: optional `TypeObject` metadata is skipped.** Generating the
   type emits a non-fatal warning
   (`TypeObject emission skipped — UnsupportedTypeSpec("map (inline IDL map)")`).
   The XCDR2 **wire codec** for the map is fully generated and round-trips; only
   the auxiliary `TypeObject` (used for remote type discovery / type
   compatibility, not for same-type loopback) is omitted for inline IDL maps.

2. **`isTelemetry` type guard mis-types the array member.** The generated guard
   checks `typeof o.window !== "number"` (a scalar test) even though `window` is
   `Array<number>`. The guard is **never invoked** by the `@zerodds/node` take
   path (the FFI binding decodes via `decodeFrom`, not the guard), so it has no
   effect on the roundtrip; it would only matter to user code that calls
   `isTelemetry()` directly on a value carrying an array member.

## Reference

Companion to the [TypeScript / Node bindings](https://zerodds.de/bindings/typescript-node/)
page. Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS
implementation.
