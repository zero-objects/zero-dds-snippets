# zerodds-build-sample

Runnable companion for the `zerodds-build` crate (`crates/zerodds-build`) — a
[`prost-build`](https://docs.rs/prost-build)-style `build.rs` helper that
compiles `.idl` -> Rust via the shared `zerodds-idl-compose` pipeline
(`#include` tracking, vendor key-pragmas, default-extensibility, TypeObject
lowering), the same pipeline `zerodds-idlc` itself runs.

Compare this with the sibling [`../idlc-buildrs`](../idlc-buildrs): that one
shells out to the compiled `zerodds-idlc` binary (needs it on `PATH`) and
follows the `documentation/04-idl/idlc-handbook.md` "6 build.rs integration"
doc snippet verbatim. This one calls `zerodds-build` as a normal
`[build-dependencies]` crate — no external binary, no `PATH` requirement,
and it additionally exercises `#include` + a vendor `#pragma keylist` to
prove the composition pipeline the ad hoc `build.rs` pattern (documented in
`zerodds-idl-compose`'s crate docs) never ran.

## What the IDL demonstrates

- `idl/Pose.idl`: a plain `@appendable struct Pose`.
- `idl/Robot.idl`: `#include "Pose.idl"`, a `struct Unit { long id; Pose pose; }`,
  and `#pragma keylist Unit id` — the Cyclone/OpenSplice/RTI pragma spelling
  of `@key`, not the OMG-IDL-4.2 annotation.

`build.rs` calls:

```rust
zerodds_build::Config::new()
    .include_dir("idl")
    .strip_inner_attrs_for_include(true)
    .compile(&["idl/Robot.idl"])
```

`src/lib.rs` then `include!`s `$OUT_DIR/Robot.rs`, and the roundtrip test
asserts both that `Unit`'s wire form carries the CDR key encoding the
pragma should have produced, and that the TypeObject block
(`type_objects::UNIT`) is non-empty — proof the full five-step pipeline ran,
not a bare `zerodds_idl::parse` + `generate_rust_module` call.

## Byte-for-byte check against the CLI

`zerodds-build`'s raw output (before `strip_inner_attrs_for_include`) is
byte-identical to `zerodds-idlc generate idl/Robot.idl --rust -I idl -o <dir>`
for the same IDL — verified as part of this crate's validation, not
re-asserted here (would require the `zerodds-idlc` binary on `PATH`, which
this example deliberately does not need).

## Build & run

Needs a checked-out [ZeroDDS source tree](https://github.com/zero-objects/zero-dds)
next to this repo (path deps into `crates/zerodds-build`, `crates/cdr`,
`crates/dcps`, `crates/types`, `crates/sql-filter`).

```sh
cargo test   # roundtrip + TypeObject unit test on the lib target
cargo run --bin zerodds-build-sample-demo
```

Expected output:

```text
zerodds-build-sample-demo: 36 bytes on the wire, roundtripped Unit { id: 7, pose: Pose { x: 1.0, y: 2.0, theta: 0.5 } }
```

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
