# crate-hpack

The website's docs/hpack.html usage snippet, made runnable.
RFC 7541: three header fields survive encode -> decode unchanged.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-hpack`.

Output of the run this README was written from:

```
hpack: 3 header fields in 27 bytes, roundtripped
```

## Reference

- Explained on the website: [HPACK](https://zerodds.org/docs/hpack.html)
- This example in the repository: [zero-dds-snippets/crate-hpack](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-hpack)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
