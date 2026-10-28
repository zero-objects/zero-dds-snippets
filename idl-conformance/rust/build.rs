// SPDX-License-Identifier: Apache-2.0
//! The type is generated here, from `idl/20_mixed_combo.idl`.
//!
//! It used to be a checked-in `generated/20_mixed_combo.rs`, written by
//! `zerodds-idl-rust` and calling into `zerodds-cdr`. Neither crate is
//! in the workspace any more, so the file named two paths that do not
//! resolve and `cargo` refused the manifest before it compiled a line.
//!
//! Generating it removes the way it went stale: a checked-in emission
//! is a copy of what the generator said once, and nothing compares it
//! with what the generator says now. `dds_type_impls` writes the
//! `DdsType` impl from the same model as the codec, so the announced
//! `TypeObject` and the octets on the wire cannot disagree.
fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .include_dir("idl")
        .dds_type_impls(true)
        .compile(&["idl/20_mixed_combo.idl"])
}
