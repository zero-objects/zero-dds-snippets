// Runtime driver for app.ts (the verbatim §quickstart snippet), run with bare
// `node` so the CommonJS `ws` package interops natively. It:
//   1. polyfills `globalThis.WebSocket` with the Node `ws` client,
//   2. shims `fetch` for the local wasm file: URL (the web wasm target boots via
//      `fetch`, which the browser serves natively),
//   3. stands up an in-process stand-in for the bridge daemon on port 9001 that
//      speaks what @zerodds/wasm speaks to `zerodds-ws-bridged`
//      (zerodds-ws-bridge-1.0): one connection per topic on its path
//      `/topics/<slug>` (§4.2), and binary frames of one sample each, `ZDB1`,
//      flags, encapsulation header, payload (§4.3). A frame a client sends is
//      delivered to every connection on the same path, the sender included, as
//      the daemon's DDS reader receives what its writer wrote,
//   4. imports the compiled app (build/app.js) unchanged.
//
// Run:  npm install && npm run build && npm run start

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

const SUBPROTOCOL = 'zerodds-ws-bridge-1.0';
const paths = new Map();
const wss = new WebSocketServer({
    port: 9001,
    handleProtocols: (protocols) => (protocols.has(SUBPROTOCOL) ? SUBPROTOCOL : false),
});
await new Promise((r) => wss.on('listening', () => r()));
wss.on('connection', (ws, request) => {
    paths.set(ws, request.url);
    ws.on('close', () => paths.delete(ws));
    ws.on('message', (frame, isBinary) => {
        // Control messages are text frames (§4.4); the client sends none here.
        if (!isBinary || frame.length < 9 || frame.subarray(0, 4).toString() !== 'ZDB1') return;
        for (const [client, path] of paths) {
            if (path === request.url) client.send(frame, { binary: true });
        }
    });
});

await import('./build/app.js');

setTimeout(() => { wss.close(); process.exit(0); }, 4000);
