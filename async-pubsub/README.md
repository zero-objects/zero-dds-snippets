# async-pubsub

A fully async publish/subscribe roundtrip built on `zerodds-dcps-async`. It creates one participant, a `RawBytes` topic `"Chatter"`, a writer and a reader, `.await`s the discovery match in both directions, writes `b"hello"`, and `.await`s `reader.take(..)` to print the received sample. Proves the `async`/`.await` DCPS surface (Tokio runtime) end to end.

## Build & run

In the demo dir:

```sh
cargo run
```

No features to enable. The `Cargo.toml` pulls `zerodds-dcps-async` and `zerodds-dcps` from crates.io (`1.0.0-rc.6`) plus `tokio` with `full`; the binary runs the whole exchange in one process.

## Reference

- Explained on the website: [Rust](https://zerodds.org/bindings/rust/)
- This example in the repository: [zero-dds-snippets/async-pubsub](https://github.com/zero-objects/zero-dds-snippets/tree/main/async-pubsub)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
