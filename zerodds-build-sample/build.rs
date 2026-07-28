//! `zerodds-build`'s own end-to-end demo: `Config::new().include_dir(...)`
//! + `#include`/vendor-keylist-pragma-bearing IDL, no CLI on `PATH`
//! needed and no shelling out — compare with `../idlc-buildrs/build.rs`
//! (the doc-companion pattern that shells out to the `zerodds-idlc`
//! binary).

fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .include_dir("idl")
        .strip_inner_attrs_for_include(true)
        .compile(&["idl/Robot.idl"])
}
