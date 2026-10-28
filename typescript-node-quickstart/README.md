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

- Explained on the website: [TypeScript (Node.js)](https://zerodds.org/bindings/typescript-node/)
- This example in the repository: [zero-dds-snippets/typescript-node-quickstart](https://github.com/zero-objects/zero-dds-snippets/tree/main/typescript-node-quickstart)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
