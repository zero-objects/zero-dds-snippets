# runtime-config

The runnable companion to **[RuntimeConfig](https://github.com/zero-objects/zero-dds/blob/main/documentation/03-configuration/runtime-config.md)**.

`main()` builds the full field-list `RuntimeConfig` struct literal from the
top of the page (`tick_period`, `spdp_period`, `spdp_multicast_group`,
`multicast_interface`, `announce_secure_endpoints`, `wlp_period`,
`participant_lease_duration`, `observability: null_sink()`) and starts a
real `DcpsRuntime` with it.

## The three "Common combinations" recipes

The page's "Common combinations" section (further down) shows three
one-field-deep variations on the same struct. Rather than three near-empty
sibling projects, they are folded into this companion as functions —
compiled (and therefore drift-checked) as part of the same `cargo build`,
each doc-commented with the page anchor it covers:

| Function | Doc anchor |
|---|---|
| `sandbox_test_config()` | "Sandbox / test" (line 93) — `RuntimeConfig::default()` |
| `production_with_monitoring_config()` | "Production server with monitoring" (line 101) — `observability: Arc::new(StderrJsonSink::new())` |
| `hard_realtime_config()` | "Hard real-time" (line 110) — `tick_period`/`spdp_period`/`participant_lease_duration` tuned |

Only the full field-list config is actually started against a live
`DcpsRuntime`; the three recipes vary fields the full config (or
[`observability-stderr-sink`](../observability-stderr-sink)) already
exercises live, so proving they compile is sufficient here.

## Build & run

Needs a checked-out [ZeroDDS source tree](https://github.com/zero-objects/zero-dds)
next to this repo (path deps into `crates/dcps`, `crates/foundation`,
`crates/rtps`).

```sh
cargo run
```

Expected output:

```text
runtime-config: DcpsRuntime started on domain 11 with the full field-list config
```

## Reference

This is the runnable companion to the code snippets on
**[documentation/03-configuration/runtime-config.md](https://github.com/zero-objects/zero-dds/blob/main/documentation/03-configuration/runtime-config.md)**.

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
