//! `zerodds-build`'s generated-code consumption side: `idl/Robot.idl`
//! `#include`s `idl/Pose.idl` and marks `Unit::id` `@key` via a
//! `#pragma keylist` (Cyclone/OpenSplice/RTI spelling) instead of the
//! OMG-IDL-4.2 `@key` annotation — both only reach the generated code
//! because `zerodds-build` runs the same composition pipeline
//! `zerodds-idlc` runs (`zerodds-idl-compose`), not a thinner ad hoc
//! `zerodds_idl::parse` + `generate_rust_module` call.
//!
//! The generated `Robot.rs` (from `build.rs`) carries file-level
//! `#![allow(...)]` inner attributes; `include!` splices those tokens
//! into *this* file, where an inner attribute is only legal as the very
//! first item — so `build.rs` calls `Config::strip_inner_attrs_for_include
//! (true)` and the include! below is wrapped in its own module with the
//! same allows re-applied as outer attributes (same pattern as
//! `../idlc-buildrs/src/lib.rs`, which strips the attributes by hand
//! since it shells out to the CLI directly).
#[allow(
    clippy::too_many_lines,
    clippy::useless_conversion,
    unused_imports,
    non_snake_case
)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/Robot.rs"));
}

pub use generated::{Pose, Unit};

#[cfg(test)]
mod tests {
    use super::{Pose, Unit};

    #[test]
    fn generated_unit_roundtrips_and_carries_typeobject() {
        use zerodds_dcps::DdsType;

        let unit = Unit {
            id: 7,
            pose: Pose {
                x: 1.0,
                y: 2.0,
                theta: 0.5,
            },
        };

        let mut bytes = Vec::new();
        unit.encode(&mut bytes).expect("encode");
        let decoded = Unit::decode(&bytes).expect("decode");
        assert_eq!(decoded, unit);
        assert_eq!(Unit::TYPE_NAME, "Unit");

        // TypeObject emission (default-on) — proves the composition
        // pipeline's TypeObject lowering step ran, not just parse+emit.
        assert!(!super::generated::type_objects::UNIT.is_empty());
    }
}
