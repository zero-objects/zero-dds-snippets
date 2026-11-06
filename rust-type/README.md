# `rust_type`: a checked Rust type for an IDL type

`zerodds-build`'s `rust_type` holds an IDL declaration as a Rust type of
your own (GitHub #33). Here the IDL `typedef string<12> Dtg;` is a
date-time group (`src/dtg.rs`) that cannot hold an invalid one, and the
key `uint32 id` is a `NonZeroU32`. A C or C++ peer built from
`idl/track.idl` sees the `string` and the `uint32` and reads the same
octets.

```rust
// build.rs
zerodds_build::Config::new()
    .rust_type("app::Dtg", "crate::dtg::Dtg")
    .rust_type("app::Track.id", "core::num::NonZeroU32")
    .dds_type_impls(true)
    .compile(&["idl/track.idl"])
```

`Dtg` implements `zerodds_wire::WireRepr` with `type Idl = str`: it writes
with `Display` straight into the sample's buffer and reads from the `&str`
the received sample lends. `zerodds-wire` carries the impl for
`NonZeroU32`.

```sh
cargo run
```

prints the octets of a sample, reads it back, and shows two samples
rejected with their reasons: a time that is no date-time group, and a key
of zero.

## Dependencies

The crates of GitHub #33 (`zerodds-codec`, `zerodds-derive`, `rust_type`
in `zerodds-build`) are newer than the latest crates.io release. The path
dependencies reach into a ZeroDDS source tree that holds this repository,
the layout of `ci/run-examples.sh` (see the repository README). With RC8
the example switches to the published crates.
