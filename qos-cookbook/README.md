# qos-cookbook

A compile-checked collection of the QoS recipes from the cookbook page. Each `snippet_N` builds a real `DataWriterQos`/`DataReaderQos`/`PublisherQos` value: best-effort volatile (`snippet_0`), reliable transient-local (`snippet_1`), exclusive ownership + strength + manual liveliness for primary/backup failover (`snippet_2`), deadline + latency-budget + reliable for a 100 Hz control loop (`snippet_3`), and partition-scoped publishing (`snippet_4`). It exists to prove every published QoS snippet still matches the live `zerodds-qos` API.

## Build & run

In the demo dir:

```sh
cargo run
```

`main` is empty — the value is that it *compiles*, pinning the QoS field names/types. Uses path deps into the local zero-dds checkout (`../../crates/dcps`, `../../crates/qos`) until these APIs ship on crates.io.

## Reference

This is the runnable companion to the code snippet on
**[QoS cookbook](https://zerodds.de/topics/qos-cookbook/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
