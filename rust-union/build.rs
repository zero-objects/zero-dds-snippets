// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 ZeroDDS Contributors
//! The IDL route: the reporter's `idl/message_payload.idl` through
//! `zerodds-build`, for `src/main.rs` to compare the derived types with.
fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .dds_type_impls(true)
        .compile(&["idl/message_payload.idl"])
}
