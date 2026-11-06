// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 ZeroDDS Contributors
//! The IDL stays the one definition of the wire; the Rust holds checked
//! types. `app::Dtg` is a `crate::dtg::Dtg` wherever it is used, and the
//! key `app::Track.id` is a `NonZeroU32`.
fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .rust_type("app::Dtg", "crate::dtg::Dtg")
        .rust_type("app::Track.id", "core::num::NonZeroU32")
        .dds_type_impls(true)
        .compile(&["idl/track.idl"])
}
