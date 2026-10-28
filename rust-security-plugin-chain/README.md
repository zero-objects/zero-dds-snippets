# rust-security-plugin-chain

Reproduces two QoS recipes from the Security Plugin-Chain Cookbook verbatim. `pki_qos()` builds a `DomainParticipantQos` carrying the `dds.sec.auth.*` PKI trust-anchor properties (identity CA, participant cert, private key); `audit_log_qos()` builds one carrying the `dds.sec.log.*` fan-out audit-logging properties. `main` builds both and wires the PKI QoS through a real participant via `DomainParticipantFactory::create(0).with_qos(qos).build()`.

## Build & run

In the demo dir:

```sh
cargo run
```

The `security` feature on `zerodds-dcps` is already enabled in `Cargo.toml`. The participant loads all six DDS-Security files (the three of recipe 1 and the Access Control plugin's `permissions_ca`, `governance`, `permissions`) from the directory `ZERODDS_PKI` names: `ca-cert.pem`, `part-cert.pem`, `part-key.pem`, `permissions-ca-cert.pem`, `governance.p7s`, `permissions.p7s`, with governance and permissions covering domain 0. Uses path deps into the local zero-dds checkout until these APIs ship on crates.io.

## Reference

- Explained on the website: [Security Plugin-Chain Cookbook](https://zerodds.org/topics/security-plugin-chain/)
- This example in the repository: [zero-dds-snippets/rust-security-plugin-chain](https://github.com/zero-objects/zero-dds-snippets/tree/main/rust-security-plugin-chain)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
