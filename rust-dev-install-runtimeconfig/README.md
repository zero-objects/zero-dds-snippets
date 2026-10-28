# rust-dev-install-runtimeconfig

The dev-install `RuntimeConfig` tuning snippet, verbatim. It builds a `RuntimeConfig` overriding `tick_period` (20 ms) and `spdp_period` (100 ms), then creates a participant with `create_participant_with_config`. Proves the runtime-tuning constructor path from the page compiles and runs against the live API.

## Build & run

In the demo dir:

```sh
cargo run
```

No features to enable. The participant is created and dropped — there's no output; the point is that the config + constructor type-check and run. Uses a path dep into the local zero-dds checkout (`../../crates/dcps`).

## Reference

- Explained on the website: [Dev Install](https://zerodds.org/topics/dev-install/)
- This example in the repository: [zero-dds-snippets/rust-dev-install-runtimeconfig](https://github.com/zero-objects/zero-dds-snippets/tree/main/rust-dev-install-runtimeconfig)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
