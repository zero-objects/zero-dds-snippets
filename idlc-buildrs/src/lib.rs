//! Verbatim from documentation/04-idl/idlc-handbook.md ("6 build.rs
//! integration (Rust)") — the `include!` fence right after the build.rs
//! one, `Robot.idl` → `Robot.rs`. If this stops compiling, either the
//! `zerodds-idlc generate --rust` invocation in `build.rs` or the
//! generated code's API drifted from the codebase.

// `clippy::useless_conversion` is a pre-existing zerodds-idl-rust codegen
// quirk (the bounded-string-length error arm always does a needless
// `.into()` on an already-`EncodeError`-typed value) — unrelated to this
// doc's build.rs/include! pattern, so allowed here rather than fixed
// (fixing it touches the shared Rust codegen backend, out of scope for a
// documentation/** companion).
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
    fn generated_pose_roundtrips() {
        use zerodds_dcps::DdsType;

        let pose = Pose {
            robot_id: "R2D2".into(),
            x: 1.0,
            y: 2.0,
            theta: 0.5,
        };

        let mut bytes = Vec::new();
        pose.encode(&mut bytes).expect("encode");
        let decoded = Pose::decode(&bytes).expect("decode");
        assert_eq!(decoded, pose);
        assert_eq!(Pose::TYPE_NAME, "Robot::Pose");
    }
}
