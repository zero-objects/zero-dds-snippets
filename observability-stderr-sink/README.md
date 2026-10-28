# observability-stderr-sink

The runnable companion to **[Observability](https://github.com/zero-objects/zero-dds/blob/main/documentation/03-configuration/observability.md)**
— Layer 2 ("Sink-based events"), "Wire it up". Starts a `DcpsRuntime` with
`RuntimeConfig.observability = Arc::new(StderrJsonSink::new())`, verbatim
from the doc, then registers a writer and a reader so the two
`*.created` lifecycle events the doc shows as sample stderr output actually
fire.

## Build & run

Needs a checked-out [ZeroDDS source tree](https://github.com/zero-objects/zero-dds)
next to this repo (path deps into `crates/dcps`, `crates/foundation`,
`crates/qos`, `crates/rtps`, `crates/types`).

```sh
cargo run
```

Expected stderr (one JSON line per lifecycle event, order may vary):

```json
{"level":"info","component":"dcps","name":"user_writer.created","attrs":{"topic":"Telemetry","type":"Robot::Pose","reliable":"true"}}
{"level":"info","component":"dcps","name":"user_reader.created","attrs":{"topic":"Commands","type":"Robot::Cmd"}}
```

Domain 9, so it does not collide with the domain-1 `getting-started-first-publisher`
companion if both run concurrently.

## Reference

- No page on the website explains this example yet; this README is the whole of it.
- This example in the repository: [zero-dds-snippets/observability-stderr-sink](https://github.com/zero-objects/zero-dds-snippets/tree/main/observability-stderr-sink)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
