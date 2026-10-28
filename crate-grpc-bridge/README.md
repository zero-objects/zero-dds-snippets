# crate-grpc-bridge

The website's docs/grpc-bridge.html usage snippet, made runnable.
The gRPC length-prefixed message framing: 1 compression byte + 4-byte
big-endian length + payload.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-grpc-bridge`.

Output of the run this README was written from:

```
grpc-bridge: 15 bytes framed, flag 0, payload "hello-grpc"
```

## Reference

- Explained on the website: [gRPC bridge](https://zerodds.org/docs/grpc-bridge.html)
- This example in the repository: [zero-dds-snippets/crate-grpc-bridge](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-grpc-bridge)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
