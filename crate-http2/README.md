# crate-http2

The website's docs/http2.html usage snippet, made runnable.
RFC 9113 section 4.1: a 9-byte frame header plus an 8-byte PING payload.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-http2`.

Output of the run this README was written from:

```
http2: 17 bytes consumed, frame type Ping
```

## Reference

- Explained on the website: [HTTP/2](https://zerodds.org/docs/http2.html)
- This example in the repository: [zero-dds-snippets/crate-http2](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-http2)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
