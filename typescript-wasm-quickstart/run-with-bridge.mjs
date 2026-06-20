// Runtime driver for app.ts (the verbatim §quickstart snippet), run with bare
// `node` so the CommonJS `ws` package interops natively. It:
//   1. polyfills `globalThis.WebSocket` with the Node `ws` client,
//   2. shims `fetch` for the local wasm file: URL (the web wasm target boots via
//      `fetch`, which the browser serves natively),
//   3. stands up an in-process websocket-bridge (subscribe/publish/notify echo)
//      on port 9001,
//   4. imports the compiled app (build/app.js) unchanged.
//
// Run:  npx tsc src/app.ts --outDir build --module ESNext --target ES2022 \
//            --moduleResolution Bundler --skipLibCheck --lib ES2022,DOM
//       node run-with-bridge.mjs

import { WebSocket, WebSocketServer } from 'ws';
import { readFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';

globalThis.WebSocket = WebSocket;

const realFetch = globalThis.fetch;
globalThis.fetch = async (input, init) => {
    const url = typeof input === 'string' ? input : String(input?.url ?? input);
    if (url.startsWith('file:') && url.endsWith('.wasm')) {
        const bytes = await readFile(fileURLToPath(url));
        return new Response(bytes, { headers: { 'content-type': 'application/wasm' } });
    }
    return realFetch(input, init);
};

const subs = new Map();
const wss = new WebSocketServer({ port: 9001 });
await new Promise((r) => wss.on('listening', () => r()));
wss.on('connection', (ws) => {
    subs.set(ws, new Set());
    ws.on('message', (raw) => {
        const msg = JSON.parse(raw.toString());
        if (msg.op === 'subscribe') {
            subs.get(ws).add(msg.topic);
        } else if (msg.op === 'publish') {
            for (const client of wss.clients) {
                if (subs.get(client)?.has(msg.topic)) {
                    client.send(JSON.stringify({
                        op: 'notify', topic: msg.topic, data: msg.data, subscription_id: 'sub-x',
                    }));
                }
            }
        }
    });
});

await import('./build/app.js');

setTimeout(() => { wss.close(); process.exit(0); }, 4000);
