# typescript-node-website-snippet

The verbatim `@zerodds/node` code blocks as they appear on the website, kept here so
they stay type-correct against the real binding. `hello.ts` is the bytes pub/sub
quickstart, `async.ts` is the `writeAsync`/`takeAsync`/`streamSamples` block (using
`declare`d `DataWriter`/`DataReader`), and `typed.ts` plus `gen/temperature.ts` is the
typed-topic block over the codegen-emitted `Temperature` TypeSupport. This is a pure
`tsc --noEmit` type-check fixture — no runtime, no `package.json`.

## Build & run

Build the native binding once in the ZeroDDS `crates/ts-node` so `@zerodds/node`
resolves:

```sh
cd crates/ts-node && npm install && npm run build
```

Then type-check the snippets from this demo directory:

```sh
npx tsc --noEmit -p tsconfig.json   # checks hello.ts, async.ts, typed.ts
```

## Reference

These are the verbatim website snippets; see the
**[bindings overview](https://zerodds.de/bindings/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
