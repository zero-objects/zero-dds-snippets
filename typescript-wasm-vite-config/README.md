# typescript-wasm-vite-config

The verbatim Vite configuration snippet for serving `@zerodds/wasm` in a browser dev
build (`vite.config.ts`). It excludes `@zerodds/wasm` from Vite's dependency
pre-bundling (`optimizeDeps.exclude`) so the `.wasm` asset loads correctly, and allows
the dev server to serve the parent directory (`server.fs.allow`). Kept here as a
type-checked fixture so the config block on the website stays valid.

## Build & run

This is a config snippet, not a running program — it is verified by type-checking only.
From this demo directory:

```sh
npx tsc --noEmit -p tsconfig.json   # checks vite.config.ts
```

Drop the `vite.config.ts` contents into a real Vite project that depends on
`@zerodds/wasm` (built via `npm run build` in the ZeroDDS `crates/ts-wasm`).

## Reference

This is the runnable companion to the code snippet on
**[TypeScript / WASM bindings](https://zerodds.de/bindings/typescript-wasm/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
