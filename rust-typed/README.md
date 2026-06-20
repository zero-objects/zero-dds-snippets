# rust-typed

Demonstrates typed topics via `#[derive(DdsType)]`. A `Temperature` struct (`celsius: i32`, `sensor_id: String`) is annotated with `#[dds(type_name = "sensor_msgs::msg::Temperature")]`, then used to create a typed topic, a typed writer, and write a `Temperature` value — the struct and usage lines are copied verbatim from the bindings page. Proves the derive macro (`zerodds-cdr-derive`) and the `DdsType` trait (`zerodds-dcps`) coexist and produce a working typed wire path.

## Build & run

In the demo dir:

```sh
cargo run
```

No features to enable. Note the writer needs a runtime, so the demo uses an online participant (`create_participant`, not the offline variant). Path deps into the local zero-dds checkout pull `zerodds-dcps`, `zerodds-cdr-derive`, and `zerodds-cdr` (the derive expands to `zerodds-cdr` calls) until these APIs ship on crates.io.

## Reference

This is the runnable companion to the code snippet on
**[Rust bindings](https://zerodds.de/bindings/rust/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
