// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 ZeroDDS Contributors
//! The IDL route of the same type: `idl/track.idl` is the IDL the derived
//! `Track` exports (`src/main.rs` checks that it still is), compiled with
//! the same date-time group and `NonZeroU32` for its key.
fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .rust_type("app::Track.at", "crate::dtg::Dtg")
        .rust_type("app::Track.id", "core::num::NonZeroU32")
        .dds_type_impls(true)
        .compile(&["idl/track.idl"])
}
