//! `zerodds-build`'s own end-to-end demo: `Config::new().include_dir(...)`
//! over `#include`/vendor-keylist-pragma-bearing IDL, with no CLI on
//! `PATH` and no shelling out.
//!
//! `dds_type_impls(true)` adds the `zerodds_dcps::DdsType` impl beside
//! the codec, from the same model — the type name, the extensibility,
//! the key from `#pragma keylist`, and the `TypeObject` a peer asks for
//! over TypeLookup. Without it the generated module is codec-only and
//! `create_topic` has nothing to take.

fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .include_dir("idl")
        .dds_type_impls(true)
        .compile(&["idl/Robot.idl"])
}
