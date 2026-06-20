# typescript-node-typed

IDL-to-TypeScript typed topics on `@zerodds/node`. `src/gen/temperature.ts` is the
`idlc ts` codegen output for `struct Temperature { long celsius; string sensor_id; }`
— the mapped interface plus a `TemperatureTypeSupport` const implementing the DDS-TS
TypeSupport contract (XCDR2 encode/decode). `src/typed.ts` uses it via
`createTypedTopic` / `createTypedWriter` to publish a strongly-typed `Temperature`
sample, proving the generated XCDR2 codec wires into the typed writer.

## Build & run

Build the native binding once in the ZeroDDS `crates/ts-node`:

```sh
cd crates/ts-node && npm install && npm run build
```

Then, from this demo directory:

```sh
npm run typecheck   # tsc --noEmit -p tsconfig.json
npm run start       # tsx src/typed.ts
```

## Reference

This is the runnable companion to the code snippet on
**[TypeScript / Node bindings](https://zerodds.de/bindings/typescript-node/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
