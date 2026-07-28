# security-governance-gate

The runnable companion to **[Security](https://github.com/zero-objects/zero-dds/blob/main/documentation/03-configuration/security.md)**
— "Code: wiring up the gate". Parses the page's own "Governance XML"
example (`governance.xml` here, copied verbatim from the same page —
encrypt everything on domain 0), builds a `SharedSecurityGate` from it plus
an `AesGcmCryptoPlugin`, and starts a real `DcpsRuntime` with
`RuntimeConfig.security` set.

## Feature gate

`RuntimeConfig.security` only exists when `zerodds-dcps` is built with the
`security` feature (`#[cfg(feature = "security")]` in
`crates/dcps/src/runtime.rs`). This companion's `Cargo.toml` enables it
directly (`zerodds-dcps = { ..., features = ["security"] }`), so
`cargo run` here always builds with security on — no extra flags needed
for this project.

## Build & run

Needs a checked-out [ZeroDDS source tree](https://github.com/zero-objects/zero-dds)
next to this repo (path deps into `crates/dcps`, `crates/rtps`,
`crates/security-runtime`, `crates/security-permissions`,
`crates/security-crypto`).

```sh
cargo run
```

Expected output:

```text
security-governance-gate: DcpsRuntime started on domain 0 with the SharedSecurityGate wired
```

## Reference

This is the runnable companion to the code snippet on
**[documentation/03-configuration/security.md](https://github.com/zero-objects/zero-dds/blob/main/documentation/03-configuration/security.md)**.

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
