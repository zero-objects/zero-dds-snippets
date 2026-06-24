//! Front-to-back round-trip of a **complex** message — nested structs, a
//! sequence of discriminated unions, a map, an enum, an optional member and a
//! bounded string — under BOTH wire representations: **XCDR2** (the DDS default)
//! and **XCDR1** (classic CDR).
//!
//! `Sensor` is generated from `demo.idl` by the ZeroDDS IDL→Rust backend
//! (`build.rs`, the `idlc --rust` path). Its `DdsType` impl carries the real
//! per-representation framing: under XCDR2 the `@appendable` aggregates get a
//! DHEADER and 8-byte primitives cap at 4-byte alignment; under XCDR1 there is
//! no DHEADER and natural 8-byte alignment. The same value must survive a
//! round-trip in either representation, byte-for-byte identical to what every
//! other ZeroDDS language binding produces.

use zerodds_dcps::DdsType;

// The build script writes the generated full-DdsType binding here. Wrapped in a
// module so its imports don't clash; its `#![...]` inner attrs were stripped.
#[allow(non_snake_case, unused_imports, clippy::all)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/sensor.rs"));
}
use generated::{Mode, Reading, Sensor, Vec3};

fn sample(note: Option<&str>) -> Sensor {
    let mut calibration = ::std::collections::BTreeMap::new();
    calibration.insert(1u32, Vec3 { x: 1.0, y: 0.0, z: 0.0 });
    calibration.insert(2u32, Vec3 { x: 0.0, y: 1.0, z: 0.0 });
    Sensor {
        id: 7,
        name: "lidar-front".to_string(),
        mode: Mode::ACTIVE,
        origin: Vec3 { x: 0.1, y: -0.2, z: 1.5 },
        // a sequence mixing all three union arms
        readings: vec![
            Reading::Pose(Vec3 { x: 1.0, y: 2.0, z: 3.0 }),
            Reading::Scalar(42.5),
            Reading::Raw(0xAB),
        ],
        calibration,
        note: note.map(str::to_string),
    }
}

fn check(
    label: &str,
    s: &Sensor,
    encode: impl Fn(&Sensor) -> Vec<u8>,
    decode: impl Fn(&[u8]) -> Sensor,
) {
    let bytes = encode(s);
    let back = decode(&bytes);
    assert_eq!(&back, s, "{label}: round-trip mismatch");
    println!("  [{label}] {} bytes — round-trip OK", bytes.len());
}

fn main() {
    for (desc, note) in [
        ("optional present", Some("calibrated 2026-06-24")),
        ("optional absent", None),
    ] {
        let s = sample(note);
        println!("{desc}:");
        check(
            "XCDR2",
            &s,
            |s| {
                let mut b = Vec::new();
                s.encode(&mut b).expect("xcdr2 encode");
                b
            },
            |b| Sensor::decode(b).expect("xcdr2 decode"),
        );
        check(
            "XCDR1",
            &s,
            |s| {
                let mut b = Vec::new();
                s.encode_xcdr1(&mut b).expect("xcdr1 encode");
                b
            },
            |b| Sensor::decode_xcdr1(b).expect("xcdr1 decode"),
        );
    }
    println!("\nall complex-type round-trips pass under both XCDR2 and XCDR1 ✓");
}
