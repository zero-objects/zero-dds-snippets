# rust-transport-tuning

Reproduces the four `RuntimeConfig { user_transports: vec![...] }` recipes from the Transport-Tuning Cookbook verbatim: TCP for NAT traversal (`TcpV4`), shared-memory in-box IPC with UDP fallback (`Shm` + `UdpV4`), UDS for container sidecars (`Uds`), and multi-transport failover (`Shm` + `Uds` + `UdpV4`). `main` builds all four and asserts/prints their `user_transports`, proving the published `UserTransportKind` preference-order snippets compile as shown.

## Build & run

In the demo dir:

```sh
cargo run
```

The `same-host-shm` and `same-host-uds` features on `zerodds-dcps` (which gate `UserTransportKind::Shm`/`Uds`) are already enabled in `Cargo.toml`, so it builds as published. Uses path deps into the local zero-dds checkout until these APIs ship on crates.io.

## Reference

This is the runnable companion to the code snippet on
**[Transport tuning](https://zerodds.de/topics/transport-tuning/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
