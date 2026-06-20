# rust-security-plugin-chain

Reproduces two QoS recipes from the Security Plugin-Chain Cookbook verbatim. `pki_qos()` builds a `DomainParticipantQos` carrying the `dds.sec.auth.*` PKI trust-anchor properties (identity CA, participant cert, private key); `audit_log_qos()` builds one carrying the `dds.sec.log.*` fan-out audit-logging properties. `main` builds both and wires the PKI QoS through a real participant via `DomainParticipantFactory::create(0).with_qos(qos).build()`.

## Build & run

In the demo dir:

```sh
cargo run
```

The `security` feature on `zerodds-dcps` is already enabled in `Cargo.toml`. The `file:/etc/zerodds/...` cert paths are illustrative property strings (the QoS is built, not loaded), so no PKI files need to exist. Uses path deps into the local zero-dds checkout until these APIs ship on crates.io.

## Reference

This is the runnable companion to the code snippet on
**[Security plugin chain](https://zerodds.de/topics/security-plugin-chain/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
