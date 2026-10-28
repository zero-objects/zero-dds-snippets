//! Verbatim from documentation/04-idl/idlc-handbook.md ("6 build.rs
//! integration (Rust)") — the `include!` fence right after the build.rs
//! one, `Robot.idl` → `Robot.rs`. If this stops compiling, either the
//! `zerodds_build::Config` call in `build.rs` or the generated code's
//! API drifted from the codebase.

// `include!` splices the generated tokens into this file, so the lint
// allows go on the wrapping module as outer attributes.
#[allow(
    clippy::too_many_lines,
    clippy::useless_conversion,
    unused_imports,
    non_snake_case
)]
mod robot {
    include!(concat!(env!("OUT_DIR"), "/Robot.rs"));
}
pub use robot::Robot;

#[cfg(test)]
mod tests {
    use super::Robot::Pose;

    #[test]
    fn generated_pose_roundtrips_and_carries_typeobject() {
        use zerodds_dcps::DdsType;

        let pose = Pose {
            robot_id: "R2D2".into(),
            x: 1.0,
            y: 2.0,
            theta: 0.5,
        };

        // Written out as `<Pose as DdsType>::…`: the generated type also
        // carries inherent `encode`/`decode` over a `zerodds_wire`
        // `Writer`/`Reader`, and the two layers share the names.
        let mut bytes = Vec::new();
        <Pose as DdsType>::encode(&pose, &mut bytes).expect("encode");
        let decoded = <Pose as DdsType>::decode(&bytes).expect("decode");
        assert_eq!(decoded, pose);
        assert_eq!(Pose::TYPE_NAME, "Robot::Pose");
        assert!(Pose::HAS_KEY, "@key string<32> robot_id must reach the type");
        assert!(
            Pose::type_object().is_some(),
            "a topic type without a TypeObject announces a name and no type \
             (XTypes 1.3 §7.6.3.2)"
        );
    }
}
