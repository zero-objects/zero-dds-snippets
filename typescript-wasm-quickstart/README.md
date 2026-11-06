# typescript-wasm-quickstart

The browser/WASM quickstart for `@zerodds/wasm` (`src/app.ts`): `await init()` boots
the WASM module, then a participant is created over a WebSocket bridge
(`createParticipantWebSocket('ws://localhost:9001', 0)`) and runs the same `Chatter`
bytes pub/sub roundtrip as the Node demo. `run-with-bridge.mjs` makes it runnable under
Node by polyfilling `WebSocket`/`fetch` and standing up an in-process stand-in for the
bridge daemon `zerodds-ws-bridged` on port 9001. It speaks the protocol of
`zerodds-ws-bridge-1.0` as the daemon does: one connection per topic on `/topics/<slug>`,
samples as binary frames (`ZDB1`, flags, encapsulation header, payload), each delivered to
every connection on the same topic.

## Build & run

Build the WASM package once in the ZeroDDS `crates/ts-wasm`:

```sh
cd crates/ts-wasm && npm install && npm run build
```

Then, from this demo directory:

```sh
npm run typecheck   # tsc --noEmit -p tsconfig.json
# run under Node via the bundled websocket bridge:
npx tsc src/app.ts --outDir build --module ESNext --target ES2022 \
    --moduleResolution Bundler --skipLibCheck --lib ES2022,DOM
node run-with-bridge.mjs
```

## Reference

- Explained on the website: [TypeScript (Browser/WASM)](https://zerodds.org/bindings/typescript-wasm/)
- This example in the repository: [zero-dds-snippets/typescript-wasm-quickstart](https://github.com/zero-objects/zero-dds-snippets/tree/main/typescript-wasm-quickstart)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
