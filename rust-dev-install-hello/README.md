# rust-dev-install-hello

The dev-install hello-world, verbatim from the page. One **online** participant, a `RawBytes` topic `"Chatter"`, a publisher+writer and subscriber+reader in the same process; it waits for the bidirectional discovery match, writes `b"hello"`, waits for data, and prints each taken sample. The canonical "your local checkout works" smoke test.

## Build & run

In the demo dir:

```sh
cargo run
```

No features to enable. Uses a path dep into the local zero-dds checkout (`../../crates/dcps`) — this is the demo you run right after wiring up a dev install.

## Reference

This is the runnable companion to the code snippet on
**[Dev install](https://zerodds.de/topics/dev-install/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
