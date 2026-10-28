# bindings-rust-main

The synchronous (blocking) counterpart to `async-pubsub`. One participant, a `RawBytes` topic `"Chatter"`, a writer and reader; it blocks on `wait_for_matched_subscription`/`wait_for_matched_publication`, writes `b"hello"`, blocks on `wait_for_data`, then prints every taken sample. Demonstrates the core synchronous DCPS API (`zerodds-dcps`) front to back.

## Build & run

In the demo dir:

```sh
cargo run
```

No features needed. `Cargo.toml` depends only on `zerodds-dcps` (`1.0.0-rc.6`, from crates.io).

## Reference

- Explained on the website: [Rust](https://zerodds.org/bindings/rust/)
- This example in the repository: [zero-dds-snippets/bindings-rust-main](https://github.com/zero-objects/zero-dds-snippets/tree/main/bindings-rust-main)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
