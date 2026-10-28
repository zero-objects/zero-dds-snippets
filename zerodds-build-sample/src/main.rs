// Demo binary using the `zerodds-build-sample` library's generated
// `Unit` (see src/lib.rs) — proves the zerodds-build Config API works
// end to end in a program, not just in `cargo test`.
//
// The trait calls are written out as `<Unit as DdsType>::…`: the
// generated type carries *inherent* `encode`/`decode` over a
// `zerodds_wire::Writer`/`Reader`, and the `DdsType` impl carries the
// same names over a `Vec<u8>` with the encapsulation's alignment cap
// already applied. An unqualified `unit.encode(&mut bytes)` would pick
// the inherent one and not compile — which is the compiler saying the
// two live at different layers, not a naming accident.
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
    <Unit as DdsType>::encode(&unit, &mut bytes).expect("encode");
    let decoded = <Unit as DdsType>::decode(&bytes).expect("decode");
    assert_eq!(decoded, unit);

    println!(
        "zerodds-build-sample-demo: {} bytes on the wire, roundtripped {:?}",
        bytes.len(),
        decoded
    );
}
