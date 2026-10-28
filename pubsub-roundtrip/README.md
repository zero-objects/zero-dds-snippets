# pubsub-roundtrip

Two participants in **one process** on the same domain, exchanging over UDP loopback. It tunes the runtime via `RuntimeConfig` (20 ms tick, 100 ms SPDP), creates a writer on one participant and a reader on the other for topic `"Chatter"`, does the event-driven discovery handshake, writes `b"Hello, DDS!"`, then waits for and prints the received sample. Proves real cross-participant SPDP discovery plus a wire roundtrip locally.

## Build & run

In the demo dir:

```sh
cargo run
```

No features to enable. Depends only on `zerodds-dcps` (`1.0.0-rc.6`, from crates.io).

## Reference

- Explained on the website: [Language bindings](https://zerodds.org/bindings/)
- This example in the repository: [zero-dds-snippets/pubsub-roundtrip](https://github.com/zero-objects/zero-dds-snippets/tree/main/pubsub-roundtrip)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
