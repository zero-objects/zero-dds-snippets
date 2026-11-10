// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 ZeroDDS Contributors
//! The cases through `zerodds-build` with `dds_type_impls(true)`; the
//! enum-prefix case of GitHub #61 with `strip_enum_prefix(true)`, its
//! naming case with `rust_naming(RustNaming::Rust)` too.
fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .dds_type_impls(true)
        .compile(&["idl/names.idl", "idl/dropped.idl", "idl/defaults.idl"])?;
    zerodds_build::Config::new()
        .dds_type_impls(true)
        .strip_enum_prefix(true)
        .compile(&["idl/enum_prefix.idl"])?;
    zerodds_build::Config::new()
        .dds_type_impls(true)
        .rust_naming(zerodds_build::RustNaming::Rust)
        .strip_enum_prefix(true)
        .compile(&["idl/rust_naming.idl"])
}
