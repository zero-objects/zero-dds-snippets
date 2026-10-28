# rust-audit-scrubbing

Reproduces the "Sensitive-Data-Scrub" wrapper recipe verbatim: a generic `ScrubbingLogger<P: LoggingPlugin>` that redacts secrets via `scrub_secrets` (a regex replacing 64+ char hex runs with `[REDACTED-HEX]`) before delegating to an inner plugin. The demo wraps a real `StderrLoggingPlugin`, logs a message containing a 64-char hex key, and asserts the redaction happened at the value level while benign messages pass through unchanged.

## Build & run

In the demo dir:

```sh
cargo run
```

No features to enable. Watch the stderr output — the leaked key prints as `[REDACTED-HEX]`. Depends on `zerodds-security`, `zerodds-security-logging` (path deps into the local zero-dds checkout) and `regex` (crates.io).

## Reference

- Explained on the website: [Audit Logging Cookbook](https://zerodds.org/topics/audit-logging/)
- This example in the repository: [zero-dds-snippets/rust-audit-scrubbing](https://github.com/zero-objects/zero-dds-snippets/tree/main/rust-audit-scrubbing)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
