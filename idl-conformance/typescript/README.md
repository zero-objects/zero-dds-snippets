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
derived from the upstream conformance combo
`tools/idlc/tests/conformance/fixtures/20_mixed_combo.idl`. The single keyed
sample combines, end-to-end over the wire:

| Feature                       | IDL member                          | Verified |
|-------------------------------|-------------------------------------|----------|
| `@key` integer                | `@key long unitId`                  | ✅ |
| `@key` + bounded string       | `@key string<32> region`            | ✅ |
| `typedef` (to `double`)       | `CurrentInAmpsType batteryCurrent`  | ✅ |
| nested struct                 | `struct Sample { long; double; }`   | ✅ |
| sequence **of struct**        | `sequence<Sample> history`          | ✅ |
| `map<string, long>`           | `map<string,long> counters`         | ✅ |
| `@optional` (present)         | `@optional double calibration`      | ✅ |
| `@optional` (absent)          | (see note)                          | ✅ |
| empty sequence / empty map    | (see note)                          | ✅ |

The roundtrip asserts deep equality of every field, including `Map` contents and
the `@optional` member, after the bytes traverse the DCPS stack.

> The `@optional`-absent, empty-sequence and empty-map cases are additionally
> covered by a second roundtrip during development; the committed example sends
> the fully-populated sample.

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

The example deliberately publishes a **subset** of the upstream combo type.
Three members of `20_mixed_combo.idl` were removed because the current `idl-ts`
XCDR2 backend cannot round-trip them — these are **real codegen bugs**, not
test-harness shortcuts. Each was confirmed by generating the type and inspecting
(or running) the emitted codec:

1. **`union` member gates off the whole struct codec.**
   The combo has `union Reading switch(Mode) { … } reading;`. `idl-ts` treats
   any struct containing a union member as non-codecable
   (`crates/idl-ts/src/lib.rs`, `typespec_xcdr2_codecable`: `Union => false`),
   so the generated `Telemetry` gets **no `TelemetryTypeSupport` at all** — only
   a comment: `// Telemetry: no XCDR2 TypeSupport — contains a fixed/any member`.
   The type cannot be published. (Standalone unions also emit no XCDR2 codec.)

2. **Fixed-array member is emitted as a scalar (data loss).**
   For `long window[4];` the generated codec emits `w.writeInt32(s.window)` and
   `r.readInt32()` — a single scalar — instead of looping over the 4 elements.
   The interface correctly types `window: Array<number>`, so the codec both
   loses data and is type-inconsistent. (Reproduced identically with
   `08_arrays.idl`: `vec[3]`, `grid[4][4]`, `Point shape[2]` all become scalar
   reads/writes; the generated file does not even typecheck.)

3. **`enum` member is encoded as its string label (throws at runtime).**
   Enums map to TS string-literal unions (`Mode.MODE_ACTIVE === "MODE_ACTIVE"`),
   but the codec emits `w.writeInt32(s.mode as unknown as number)` on the
   **string** value, which throws `XcdrError: int32 out of range: MODE_ACTIVE`.
   The generated `ModeOrdinal` / `ModeFromOrdinal` lookup maps exist but the
   codec never uses them, so an enum-typed struct member cannot round-trip.
   (Reproduced with `03_enums.idl`'s `struct UsesEnum`.)

With those three removed, the remaining seven features above all round-trip
cleanly over real DCPS. The map member also emits a non-fatal codegen warning
(`TypeObject emission skipped — map (inline IDL map)`); the XCDR2 wire codec for
maps is generated and works — only the optional `TypeObject` metadata is skipped.

## Reference

Companion to the [TypeScript / Node bindings](https://zerodds.de/bindings/typescript-node/)
page. Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS
implementation.
