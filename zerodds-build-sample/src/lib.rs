//! `zerodds-build`'s generated-code consumption side: `idl/Robot.idl`
//! `#include`s `idl/Pose.idl` and marks `Unit::id` `@key` via a
//! `#pragma keylist` (Cyclone/OpenSplice/RTI spelling) instead of the
//! OMG-IDL-4.2 `@key` annotation — both only reach the generated code
//! because `zerodds-build` runs the same composition pipeline
//! `zerodds-idlc` runs, not a thinner ad hoc parse-and-emit call.
//!
//! `include!` splices the generated tokens into *this* file, so the lint
//! allows go on the wrapping module as outer attributes (same pattern as
//! `../idlc-buildrs/src/lib.rs`).
#[allow(
    clippy::too_many_lines,
    clippy::useless_conversion,
    unused_imports,
    non_snake_case
)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/Robot.rs"));
}

pub use generated::geometry::Pose;
pub use generated::Unit;

#[cfg(test)]
mod tests {
    use super::{Pose, Unit};

    /// The round-trip, the announced type name, and the `TypeObject`
    /// that goes with it.
    ///
    /// `type_object()` returning `None` is the failure this asserts
    /// against: the endpoint would announce a topic type name and no
    /// type at all, and a peer's TypeLookup request would come back
    /// empty (XTypes 1.3 §7.6.3.2).
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
        <Unit as DdsType>::encode(&unit, &mut bytes).expect("encode");
        let decoded = <Unit as DdsType>::decode(&bytes).expect("decode");
        assert_eq!(decoded, unit);
        assert_eq!(Unit::TYPE_NAME, "Unit");

        // The `#pragma keylist` reached the model: without it the type
        // would have no key and every sample would land in one instance.
        assert!(Unit::HAS_KEY, "#pragma keylist Unit id must reach the type");

        assert!(
            Unit::type_object().is_some(),
            "the TypeObject lowering step must have run, not just parse+emit"
        );
    }
}
