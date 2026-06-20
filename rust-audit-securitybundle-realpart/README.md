# rust-audit-securitybundle-realpart

Shows the **correct** participant wiring for an audit `SecurityBundle`, fixing the docs snippet. The plugin construction (`StderrLoggingPlugin`, `JsonLinesLoggingPlugin::open`, `FanOutLoggingPlugin::new().with(..)`, `SecurityBundle::builder().logging_plugin(..).build()`) is real and verified; but the website's `DomainParticipantFactory::create(0).with_security(bundle).build()` chain does not exist. The real path — `RuntimeConfig::default().with_security_bundle(&bundle)` + `create_participant_with_config` — is what this demo runs.

## Build & run

In the demo dir:

```sh
cargo run
```

The `security` feature on `zerodds-dcps` is already enabled in `Cargo.toml`. The audit NDJSON is written under the temp dir (path printed on exit). Uses path deps into the local zero-dds checkout until these APIs ship on crates.io.

## Reference

This is the runnable companion to the code snippet on
**[Audit logging](https://zerodds.de/topics/audit-logging/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
