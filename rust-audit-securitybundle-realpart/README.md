# rust-audit-securitybundle-realpart

Runs the website's audit `SecurityBundle` snippet as written: the plugin construction (`StderrLoggingPlugin`, `JsonLinesLoggingPlugin::open`, `FanOutLoggingPlugin::new().with(..)`, `SecurityBundle::builder().logging_plugin(..).build()`) and the participant wiring `DomainParticipantFactory::create(0).with_security(bundle).build()` (feature `security` of `zerodds-dcps`). The builder is the same as `RuntimeConfig::default().with_security_bundle(&bundle)` passed to `create_participant_with_config`.

## Build & run

In the demo dir:

```sh
cargo run
```

The `security` feature on `zerodds-dcps` is already enabled in `Cargo.toml`. The audit NDJSON is written under the temp dir (path printed on exit). Uses path deps into the local zero-dds checkout until these APIs ship on crates.io.

## FIPS crypto backend (rc.4)

The DDS-Security crypto provider is a **compile-time backend swap** via the
`fips` umbrella — no code change. The default `ring` backend becomes the
FIPS-capable `aws-lc-rs` (or `wolfcrypt`) across all five security crates with a
single feature flag:

```sh
cargo run                       # default: ring
cargo run --features fips       # FIPS-validated crypto (aws-lc-rs)
```

The bundle wiring above is identical either way — only the primitives
(AES-GCM, HMAC, ECDSA/ECDH) change provider. See the
[`security-runtime`](https://github.com/zero-objects/zero-dds/tree/main/crates/security-runtime)
crate for the umbrella definition.

## Reference

- Explained on the website: [Audit Logging Cookbook](https://zerodds.org/topics/audit-logging/)
- This example in the repository: [zero-dds-snippets/rust-audit-securitybundle-realpart](https://github.com/zero-objects/zero-dds-snippets/tree/main/rust-audit-securitybundle-realpart)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
