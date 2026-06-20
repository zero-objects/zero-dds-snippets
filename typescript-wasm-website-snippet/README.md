# typescript-wasm-website-snippet

The verbatim `@zerodds/wasm` browser quickstart block as it appears on the website
(`app.ts`), kept here so it stays type-correct against the real WASM binding. It shows
the `await init()` WASM bootstrap, `createParticipantWebSocket('ws://localhost:9001', 0)`,
and a `Chatter` bytes pub/sub roundtrip. This is a pure `tsc --noEmit` type-check
fixture — no runtime, no `package.json`.

## Build & run

Build the WASM package once in the ZeroDDS `crates/ts-wasm` so `@zerodds/wasm`
resolves:

```sh
cd crates/ts-wasm && npm install && npm run build
```

Then type-check the snippet from this demo directory:

```sh
npx tsc --noEmit -p tsconfig.json   # checks app.ts
```

## Reference

This is the verbatim website snippet; see the
**[bindings overview](https://zerodds.de/bindings/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
