# idlc-buildrs

Executable companion to the [IDL compiler handbook, section 6](../../documentation/04-idl/idlc-handbook.md#6-buildrs-integration-rust).
The [build script](build.rs) calls `zerodds-build` as a library. It does
not shell out to the IDLC CLI and does not require that executable on
`PATH`.

## Checkout layout

The current [Cargo.toml](Cargo.toml) uses path dependencies into the parent
ZeroDDS checkout:

```text
zerodds/
  crates/
  documentation/
  zerodds-examples/
    idlc-buildrs/
```

The Examples repository is separate but nested at that location for these
paths and the handbook backlink. If checked out elsewhere, adapt dependency
paths before building. A sibling checkout does not satisfy the current
`../../crates/...` paths.

## What is exercised

[idl/Robot.idl](idl/Robot.idl) declares an appendable `Robot::Pose` with a
bounded string key. The complete generation call is:

```rust
fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .include_dir("idl")
        .dds_type_impls(true)
        .compile(&["idl/Robot.idl"])
}
```

It composes the schema, generates Rust declarations and inherent codecs
against `zerodds-wire`, and requests the additional `DdsType` bridge.
[src/lib.rs](src/lib.rs) includes `OUT_DIR/Robot.rs` inside a module with
outer lint attributes. The build script does not strip generated attributes
or patch source text.

The library test `generated_pose_roundtrips_and_carries_typeobject`:

- Constructs the canonical `Robot::Pose` value and round-trips it through
  explicitly qualified `DdsType::encode` and `DdsType::decode`.
- Checks the type name, key presence and nonempty TypeObject.

[src/main.rs](src/main.rs) is the corresponding demo program. The
[include/pragma companion](../zerodds-build-sample/README.md) also uses the
library API and adds cross-file composition.

## Build and run

From this example directory, with the nested checkout layout above:

```sh
cargo test -j 1
cargo run -j 1 --bin idlc-buildrs-demo
```

The source manifest selects `crates/zerodds-build`, `crates/wire`,
`crates/dcps`, `crates/types` and `crates/sql-filter` from that same tree.
There is no `crates/cdr` dependency and no crates.io RC7 availability
assumption. Use the checkout's Rust toolchain and available dependency cache.

This README was aligned with the actual source during the internal RC7
documentation pass. Those commands were not rerun by that pass. The local
test's assertions are not evidence of a live participant exchange or a
cross-vendor TypeLookup result. Record the candidate revision and actual
test result when evaluating release readiness.
