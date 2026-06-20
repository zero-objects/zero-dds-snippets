# rust-audit-factory-chain

Wires a `SecurityBundle` carrying a fan-out audit logger into a participant through the fluent `DomainParticipantFactory::create(0).with_security(bundle).build()` chain. It builds a `FanOutLoggingPlugin` that fans a `StderrLoggingPlugin` (Warning+) and a `JsonLinesLoggingPlugin` writing NDJSON (Notice+), drops them in a `SecurityBundle`, brings the participant up, and prints its domain. A `website_snippet_exact` fn holds the verbatim docs code (compiled, never run) so the published snippet can't drift.

## Build & run

In the demo dir:

```sh
cargo run
```

The `security` feature on `zerodds-dcps` is already enabled in `Cargo.toml`. The live run uses a writable temp audit path (the website snippet's `/var/log/zerodds` path needs root; the API is identical). Uses path deps into the local zero-dds checkout until these APIs ship on crates.io.

## Reference

This is the runnable companion to the code snippet on
**[Audit reference](https://zerodds.de/topics/audit-reference/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
