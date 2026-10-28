# ZeroDDS — ShapesDemo pub/sub

A standalone Rust crate. `build.rs` generates the type from
[`../ShapeType.idl`](../ShapeType.idl), the four-member `@final` ShapesDemo type
every peer in `cross-vendor-shapes` is built from. `shapes_pub` writes a
moving shape, `shapes_sub` prints every sample it takes.

```sh
cargo build --release

./target/release/shapes_sub Square 70 &           # topic, domain
./target/release/shapes_pub Square BLUE 70        # topic, color, domain
```

The `shapes_demo_*` examples of the main repository speak another
`ShapeType`: the five-member `@appendable` one of the OMG dds-rtps suite. The
two share a name and not a wire form, which is why this crate exists.

Part of **[ZeroDDS](https://zerodds.de)**.
