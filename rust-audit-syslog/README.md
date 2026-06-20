# rust-audit-syslog

The "Syslog → SIEM" recipe: `SyslogLoggingPlugin::connect("127.0.0.1:514", "zerodds", "edge-01", LogLevel::Notice)` reproduced verbatim, then wired into a `SecurityBundle` and a real participant via `RuntimeConfig::with_security_bundle` + `create_participant_with_config` (same correct wiring as the `*-realpart` demo). Because syslog is connectionless UDP, `connect` binds a local ephemeral socket and succeeds even with nothing listening on `:514`.

## Build & run

In the demo dir:

```sh
cargo run
```

The `security` feature on `zerodds-dcps` is already enabled in `Cargo.toml`. No rsyslog needs to be running for the demo to complete (UDP). Uses path deps into the local zero-dds checkout until these APIs ship on crates.io.

## Reference

This is the runnable companion to the code snippet on
**[Audit logging](https://zerodds.de/topics/audit-logging/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
