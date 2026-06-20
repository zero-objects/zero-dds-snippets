# rust-audit-loggingplugin

Implements the `zerodds_security::logging::LoggingPlugin` SPI with a tiny `DemoSink`, proving the published trait snippet matches the live API. The `log(&self, level, participant: [u8; 16], category, message)` signature is reproduced exactly as shown on the docs; the demo also supplies the `plugin_class_id()` the live trait additionally requires, then calls `log(..)` with a simulated `auth.handshake.failed` event.

## Build & run

In the demo dir:

```sh
cargo run
```

No features to enable. Depends only on `zerodds-security` via a path dep into the local zero-dds checkout (until the API ships on crates.io).

## Reference

This is the runnable companion to the code snippet on
**[Audit logging](https://zerodds.de/topics/audit-logging/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
