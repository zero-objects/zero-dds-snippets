// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 ZeroDDS Contributors
//! The reporters' IDL through `zerodds-build`, with the settings of the
//! issues: `dds_type_impls(true)`.
fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new().dds_type_impls(true).compile(&[
        "idl/issue62.idl",
        "idl/issue63.idl",
        "idl/issue64.idl",
    ])
}
