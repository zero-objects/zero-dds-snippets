# typescript-wasm-quickstart

The browser/WASM quickstart for `@zerodds/wasm` (`src/app.ts`): `await init()` boots
the WASM module, then a participant is created over a WebSocket bridge
(`createParticipantWebSocket('ws://localhost:9001', 0)`) and runs the same `Chatter`
bytes pub/sub roundtrip as the Node demo. `run-with-bridge.mjs` makes it runnable under
Node by polyfilling `WebSocket`/`fetch` and standing up an in-process echo bridge on
port 9001, proving DCPS-over-WebSocket works end-to-end.

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

This is the runnable companion to the code snippet on
**[TypeScript / WASM bindings](https://zerodds.de/bindings/typescript-wasm/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
