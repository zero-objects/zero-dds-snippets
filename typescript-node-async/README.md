# typescript-node-async

The `@zerodds/node` async/await surface: `writer.writeAsync()`, `reader.takeAsync()`
and the `reader.streamSamples()` async-iterator. `src/async.ts` holds the verbatim
website snippet (type-checked with `tsc`); `src/run.ts` drives the same three methods
deterministically — match, `writeAsync`, `takeAsync`, then publish two more and break
out of the otherwise-infinite `streamSamples` loop after consuming them.

## Build & run

Build the native binding once in the ZeroDDS `crates/ts-node`:

```sh
cd crates/ts-node && npm install && npm run build
```

Then, from this demo directory:

```sh
npm run typecheck     # tsc --noEmit -p tsconfig.json (checks src/async.ts snippet)
npm run start         # tsx src/async.ts
tsx src/run.ts        # observe writeAsync/takeAsync/streamSamples end-to-end
```

## Reference

This is the runnable companion to the code snippet on
**[TypeScript / Node bindings](https://zerodds.de/bindings/typescript-node/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
