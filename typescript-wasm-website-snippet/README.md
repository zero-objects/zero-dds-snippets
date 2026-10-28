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

- Explained on the website: [TypeScript (Browser/WASM)](https://zerodds.org/bindings/typescript-wasm/)
- This example in the repository: [zero-dds-snippets/typescript-wasm-website-snippet](https://github.com/zero-objects/zero-dds-snippets/tree/main/typescript-wasm-website-snippet)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
