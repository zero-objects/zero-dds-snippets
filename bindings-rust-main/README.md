# bindings-rust-main

The synchronous (blocking) counterpart to `async-pubsub`. One participant, a `RawBytes` topic `"Chatter"`, a writer and reader; it blocks on `wait_for_matched_subscription`/`wait_for_matched_publication`, writes `b"hello"`, blocks on `wait_for_data`, then prints every taken sample. Demonstrates the core synchronous DCPS API (`zerodds-dcps`) front to back.

## Build & run

In the demo dir:

```sh
cargo run
```

No features needed. `Cargo.toml` depends only on `zerodds-dcps` (`1.0.0-rc.3`, from crates.io).

## Reference

This is the runnable companion to the code snippet on
**[Rust bindings](https://zerodds.de/bindings/rust/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
