// Demo binary using the `zerodds-build-sample` library's generated
// `Robot::Unit` (see src/lib.rs) — proves the zerodds-build Config API
// works end to end in a program, not just in `cargo test`.
use zerodds_build_sample::{Pose, Unit};
use zerodds_dcps::DdsType;

fn main() {
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

    println!(
        "zerodds-build-sample-demo: {} bytes on the wire, roundtripped {:?}",
        bytes.len(),
        decoded
    );
}
