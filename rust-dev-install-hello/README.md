# rust-dev-install-hello

The dev-install hello-world, verbatim from the page. One **online** participant, a `RawBytes` topic `"Chatter"`, a publisher+writer and subscriber+reader in the same process; it waits for the bidirectional discovery match, writes `b"hello"`, waits for data, and prints each taken sample. The canonical "your local checkout works" smoke test.

## Build & run

In the demo dir:

```sh
cargo run
```

No features to enable. Uses a path dep into the local zero-dds checkout (`../../crates/dcps`) — this is the demo you run right after wiring up a dev install.

## Reference

- Explained on the website: [Dev Install](https://zerodds.org/topics/dev-install/)
- This example in the repository: [zero-dds-snippets/rust-dev-install-hello](https://github.com/zero-objects/zero-dds-snippets/tree/main/rust-dev-install-hello)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
