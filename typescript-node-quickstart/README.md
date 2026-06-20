# typescript-node-quickstart

The minimal `@zerodds/node` bytes pub/sub roundtrip (`src/hello.ts`). It creates a
participant on domain 0, a `Chatter` bytes topic, a writer and a reader, waits for
the two to match, then publishes `hello` and takes it back on the reader. Proves the
synchronous N-API binding works end-to-end against the real `@zerodds/node` types.

## Build & run

Build the native binding once in the ZeroDDS `crates/ts-node`:

```sh
cd crates/ts-node && npm install && npm run build
```

Then, from this demo directory:

```sh
npm run typecheck   # tsc --noEmit -p tsconfig.json
npm run start       # tsx src/hello.ts
```

## Reference

This is the runnable companion to the code snippet on
**[TypeScript / Node bindings](https://zerodds.de/bindings/typescript-node/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
