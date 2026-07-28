// Demo binary using the `idlc-buildrs` library's generated `Robot::Pose`
// (see src/lib.rs) — proves the build.rs + include! pattern from
// documentation/04-idl/idlc-handbook.md works end to end in a program, not
// just in a `cargo test`.
use idlc_buildrs::Robot::Pose;
use zerodds_dcps::DdsType;

fn main() {
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

    println!(
        "idlc-buildrs-demo: {} bytes on the wire, roundtripped {:?}",
        bytes.len(),
        decoded
    );
}
