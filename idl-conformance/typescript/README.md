# idl-conformance / typescript

A **standalone, runnable** IDL-conformance example for the ZeroDDS TypeScript
binding PSM. `run.sh` generates the typed topic from the IDL with
`zerodds-idlc generate --lang typescript`, then the program publishes one fully-populated sample over a **real DCPS participant +
Publisher/DataWriter** and reads it back through a **Subscriber/DataReader** on
the loopback transport, asserting the recovered sample is deep-equal to what was
published. This is a genuine RTPS pub→sub roundtrip via the `@zerodds/node` FFI
binding (native `libzerodds`), **not** an in-memory encode/decode.

## What IDL features are exercised

The topic type is `combo::Telemetry` in [`idl/telemetry_combo.idl`](idl/telemetry_combo.idl),
the conformance combo with explicitly qualified member type references. The single keyed
sample combines, end-to-end over the wire:

| Feature                       | IDL member                          | Verified |
|-------------------------------|-------------------------------------|----------|
| `@key` integer                | `@key long unitId`                  | ✅ |
| `@key` + bounded string       | `@key string<32> region`            | ✅ |
| **enum-typed member**         | `combo::Mode mode`                  | ✅ |
| `typedef` (to `double`)       | `CurrentInAmpsType batteryCurrent`  | ✅ |
| nested struct                 | `struct Sample { long; double; }`   | ✅ |
| sequence **of struct**        | `sequence<Sample> history`          | ✅ |
| **union member**              | `combo::Reading reading` (`switch(Mode)`) | ✅ |
| `map<string, long>`           | `map<string,long> counters`         | ✅ |
| `@optional` (present)         | `@optional double calibration`      | ✅ |
| **fixed-size array member**   | `long window[4]`                    | ✅ |

The roundtrip asserts deep equality of every field — including `Map` contents,
the `@optional` member, the `enum` member, the discriminated `union` branch and
all four `window[4]` elements — after the bytes traverse the DCPS stack. The
enum, union and fixed-array members each get an additional explicit assertion so
a regression in those codecs is unambiguous.

## Build & run

The native library and the `@zerodds/node` binding must be built in the
ZeroDDS checkout this directory sits in — or in `$ZERODDS_SRC` — (a
`libzerodds` under `target/`, and `npm install --install-links=false && npm
run build` in `crates/ts-node`, which also provides `tsc` and `tsx` in its
`node_modules/.bin`). Then, from **this** directory:

```sh
PATH="$ZERODDS_SRC/crates/ts-node/node_modules/.bin:$PATH" ./run.sh
```

`run.sh` builds `zerodds-idlc`, regenerates `gen/telemetry_combo.ts` (not
committed), type-checks the program with `tsc --noEmit` and runs it with
`tsx`. `@zerodds/node` and `@zerodds/wire` resolve through `tsconfig.json`
`paths` to `crates/ts-node/dist` and `runtime/typescript/dist`.

The typed writer and reader declare the generated TypeSupport's
extensibility, representation and CompleteTypeObject to the native library,
so the sample is framed as the XCDR2 body the codec writes.

Expected output:

```
PUBLISHED: {"unitId":4711,"region":"eu-central-1",...}
RECEIVED : {"unitId":4711,"region":"eu-central-1",...}
ROUNDTRIP OK — combo::Telemetry survived the DCPS wire intact.
```

## Reference

Companion to the [TypeScript / Node bindings](https://zerodds.de/bindings/typescript-node/)
page. Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS
implementation.
