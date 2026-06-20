# pubsub-roundtrip

Two participants in **one process** on the same domain, exchanging over UDP loopback. It tunes the runtime via `RuntimeConfig` (20 ms tick, 100 ms SPDP), creates a writer on one participant and a reader on the other for topic `"Chatter"`, does the event-driven discovery handshake, writes `b"Hello, DDS!"`, then waits for and prints the received sample. Proves real cross-participant SPDP discovery plus a wire roundtrip locally.

## Build & run

In the demo dir:

```sh
cargo run
```

No features to enable. Depends only on `zerodds-dcps` (`1.0.0-rc.3`, from crates.io).

## Reference

This demo has no dedicated website page; see the
**[Rust bindings overview](https://zerodds.de/bindings/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
