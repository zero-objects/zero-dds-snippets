# rust-audit-otlp

Constructs an `OtlpExporter` from an `OtlpConfig` (collector host/port `4318`, service name/version, 5 s timeout) and calls `flush()`, proving the OTLP exporter API surface from the docs is real. The span/histogram/event adds are shown commented (the docs elide their bodies with `/* ... */`); `flush()` on an empty buffer is a no-op returning `Ok`, so the demo runs without a live collector.

## Build & run

In the demo dir:

```sh
cargo run
```

No features to enable, and no OTLP collector required (empty-buffer flush is a no-op). Depends only on `zerodds-observability-otlp` via a path dep into the local zero-dds checkout (until the API ships on crates.io).

## Reference

This is the runnable companion to the code snippet on
**[Audit logging](https://zerodds.de/topics/audit-logging/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
