# crate-websocket-bridge

The website's docs/websocket-bridge.html usage snippet, made runnable.
RFC 6455 section 1.3 carries this exact nonce/accept pair as its own example.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-websocket-bridge`.

Output of the run this README was written from:

```
websocket-bridge: Sec-WebSocket-Accept = s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
```

## Reference

- Explained on the website: [WebSocket bridge](https://zerodds.org/docs/websocket-bridge.html)
- This example in the repository: [zero-dds-snippets/crate-websocket-bridge](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-websocket-bridge)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
