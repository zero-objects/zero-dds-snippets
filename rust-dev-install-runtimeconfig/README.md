# rust-dev-install-runtimeconfig

The dev-install `RuntimeConfig` tuning snippet, verbatim. It builds a `RuntimeConfig` overriding `tick_period` (20 ms) and `spdp_period` (100 ms), then creates a participant with `create_participant_with_config`. Proves the runtime-tuning constructor path from the page compiles and runs against the live API.

## Build & run

In the demo dir:

```sh
cargo run
```

No features to enable. The participant is created and dropped — there's no output; the point is that the config + constructor type-check and run. Uses a path dep into the local zero-dds checkout (`../../crates/dcps`).

## Reference

This is the runnable companion to the code snippet on
**[Dev install](https://zerodds.de/topics/dev-install/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
