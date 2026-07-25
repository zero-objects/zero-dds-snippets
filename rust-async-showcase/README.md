<!-- SPDX-License-Identifier: Apache-2.0 -->
# rust-async-showcase

Every function of the ZeroDDS async DCPS API (`zerodds-dcps-async`, the
`zerodds-async-1.0` spec) in one runnable walkthrough — the companion to
[issue #9 “help: async”](https://github.com/zero-objects/zero-dds/issues/9).

The async API is **runtime-agnostic**: only `futures-core` is required at its
core, so it runs on Tokio, async-std or smol. It wraps the sync DCPS API with no
state duplication and no overhead — the async newtypes share the same `Arc` as
their sync counterparts, and the reader wakes futures directly on sample arrival
(no polling in live mode).

## What it covers

| Area | Functions |
|------|-----------|
| Factory / setup | `AsyncDomainParticipantFactory::instance`, `create_participant_offline`, `create_topic`, `create_publisher`/`create_subscriber`, `create_datawriter`/`create_datareader` |
| Writer futures | `write`, `register_instance`, `dispose`, `unregister_instance`, `wait_for_matched_subscription`, `matched_subscription_count`, `qos` |
| Reader futures | `take`, `wait_for_matched_publication`, `matched_publication_count`, `qos` |
| Content filter | `create_datareader_filtered` — a content-filtered subscription (DDS 1.4 §2.2.2.5.4), only matching samples delivered |
| Streams | `take_stream` (`Stream<Item = T>`), `data_available_stream` (`Stream<Item = ()>`), `publication_matched_stream` (`Stream<Item = SubscriptionMatchedStatus>`) |
| Tokio integration | `spawn_in_tokio` — a live participant driven by the Tokio runtime |

## Requirements

This example uses the async surface that ships from **rc.7** onward (the
SampleInfo readers, content-filtered readers, and the derive `field_value` used
by the SQL filter). Until rc.7 is on crates.io it therefore builds **against the
ZeroDDS source**, not the published crates: `Cargo.toml` points the `zerodds-*`
dependencies at the [`zero-dds`](https://github.com/zero-objects/zero-dds) git
repo. From rc.7 on you can switch them to `version = "1.0.0-rc.7"`.

## Run

```sh
cargo run     # prints each step (1..13), ends with "ALL OK"
cargo test    # 11 functional tests, one per function group
```

Uses a live participant over the loopback so the writer and reader match; in
live mode the streams are waker-driven via `spawn_in_tokio`.

### Other async bindings

| Binding | Directory |
|---------|-----------|
| Rust — minimal pub/sub | [`../async-pubsub`](../async-pubsub) |
| Python (`asyncio`) | [`../python-aio`](../python-aio) |
| C# (`async`/`await`) | [`../csharp-async`](../csharp-async) |
| TypeScript (Node) | [`../typescript-node-async`](../typescript-node-async) |
