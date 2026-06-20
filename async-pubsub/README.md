# async-pubsub

A fully async publish/subscribe roundtrip built on `zerodds-dcps-async`. It creates one participant, a `RawBytes` topic `"Chatter"`, a writer and a reader, `.await`s the discovery match in both directions, writes `b"hello"`, and `.await`s `reader.take(..)` to print the received sample. Proves the `async`/`.await` DCPS surface (Tokio runtime) end to end.

## Build & run

In the demo dir:

```sh
cargo run
```

No features to enable. The `Cargo.toml` pulls `zerodds-dcps-async` and `zerodds-dcps` from crates.io (`1.0.0-rc.3`) plus `tokio` with `full`; the binary runs the whole exchange in one process.

## Reference

This is the runnable companion to the code snippet on
**[Rust bindings](https://zerodds.de/bindings/rust/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
