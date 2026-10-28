# zerodds-build-sample

Executable companion for [zerodds-build](../../crates/zerodds-build/README.md)
and the [IDL compiler handbook](../../documentation/04-idl/idlc-handbook.md#6-buildrs-integration-rust).
Like [idlc-buildrs](../idlc-buildrs/README.md), this example invokes the
build helper as a library, not the IDLC executable. Neither companion needs
`zerodds-idlc` on `PATH`.

## Checkout layout

The [Cargo.toml](Cargo.toml) expects this separate Examples repository nested
at `zerodds/zerodds-examples/`. Its `../../crates/...` dependencies and
the handbook backlink resolve against that parent ZeroDDS checkout.
For another layout, adapt those paths explicitly.

## What is exercised

- [idl/Pose.idl](idl/Pose.idl) declares `geometry::Pose`.
- [idl/Robot.idl](idl/Robot.idl) includes that file, declares `Unit`, and
  marks `Unit::id` as a key with `#pragma keylist Unit id`.
- [build.rs](build.rs) uses the composition pipeline and requests the
  generated Rust `DdsType` bridge:

```rust
fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .include_dir("idl")
        .dds_type_impls(true)
        .compile(&["idl/Robot.idl"])
}
```

[src/lib.rs](src/lib.rs) includes `OUT_DIR/Robot.rs` inside a module with
outer lint attributes, then exports `geometry::Pose` and `Unit`.
There is no `strip_inner_attrs_for_include` call or generated-text patch.

The test `generated_unit_roundtrips_and_carries_typeobject` checks that a
constructed `Unit` survives the explicitly qualified `DdsType` codec
round-trip, has type name `Unit`, has a key, and supplies a TypeObject.
It does not assert that helper output is byte-identical to CLI output.
The helper requests metadata and a runtime bridge that are not selected
by the CLI's generation flags.

The build helper emits Cargo rebuild dependencies for included files.
This example uses `zerodds-wire` and the current DCPS/types/filter crates
from the same local tree, not the removed legacy CDR emitter path.

## Build and run

From this example directory:

```sh
cargo test -j 1
cargo run -j 1 --bin zerodds-build-sample-demo
```

[src/main.rs](src/main.rs) constructs and prints the demonstration value.
The current manifest isolates this crate from the parent's Cargo workspace
and uses local path dependencies. It does not require published RC7 packages.

This README was checked against source during the internal RC7
documentation pass; that pass did not rerun these commands. A local codec
round-trip and a nonempty local TypeObject do not establish network
discovery, TypeLookup delivery, or vendor interoperability. Measure those
separately against the exact release candidate.
