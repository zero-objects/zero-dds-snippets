# rust-typed

Demonstrates typed topics. `sensor_msgs::msg::Temperature` comes from [`temperature.idl`](temperature.idl), compiled at build time by `zerodds-build` (see [`build.rs`](build.rs)); `create_topic` and `create_datawriter` take it through the generated `zerodds_dcps::DdsType` impl, and the demo writes one sample.

The `#[derive(DdsType)]` bridge earlier releases carried is gone. It stated the type twice — once as a Rust struct, once as a codec — and left `type_object()` at its default `None`, so the endpoint announced a topic type name with no type behind it and a peer's TypeLookup request came back empty (XTypes 1.3 §7.6.3.2). The generated impl carries `TYPE_NAME`, `EXTENSIBILITY`, `HAS_KEY`, the `TypeIdentifier` and the `TypeObject` from the same model as the codec, so they cannot drift apart; `main` asserts the `TypeObject` is there.

## Build & run

In the demo dir:

```sh
cargo run
```

No features to enable. The writer needs a runtime, so the demo uses an online participant (`create_participant`, not the offline variant). The path deps reach into the local zero-dds checkout — `zerodds-build` at build time, then `zerodds-dcps`, `zerodds-wire`, `zerodds-types` and `zerodds-sql-filter` — until the rc.7 crates reach crates.io.

## Reference

- Explained on the website: [Rust](https://zerodds.org/bindings/rust/)
- This example in the repository: [zero-dds-snippets/rust-typed](https://github.com/zero-objects/zero-dds-snippets/tree/main/rust-typed)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
