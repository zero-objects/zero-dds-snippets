//! Front-to-back round-trip of a **complex** message — nested structs, a
//! sequence of discriminated unions, a map, an enum, an optional member and a
//! bounded string — under BOTH wire representations: **XCDR2** (the DDS default)
//! and **XCDR1** (classic CDR).
//!
//! The trait calls are written out as `<Sensor as DdsType>::…`: the generated
//! type carries *inherent* `encode`/`decode` over a `zerodds_wire::Writer`/
//! `Reader`, and the `DdsType` impl carries the same names over a `Vec<u8>`
//! with the encapsulation applied — an unqualified `s.encode(&mut b)` picks the
//! inherent one and does not compile.
//!
//! `Sensor` is generated from `demo.idl` by `zerodds-build` (`build.rs`,
//! the same composition pipeline the `zerodds-idlc` CLI runs). Its `DdsType` impl carries the real
//! per-representation framing: under XCDR2 the `@appendable` aggregates get a
//! DHEADER and 8-byte primitives cap at 4-byte alignment; under XCDR1 there is
//! no DHEADER and natural 8-byte alignment. The same value must survive a
//! round-trip in either representation. This example does not compare the wire
//! bytes against another language binding or an independent vendor.

use zerodds_dcps::DdsType;

// The build script writes the generated full-DdsType binding here. Wrapped in a
// module so its imports don't clash; its `#![...]` inner attrs were stripped.
#[allow(non_snake_case, unused_imports, dead_code, clippy::all)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/demo.rs"));
}
use generated::{Reading, Sensor, SensorMode, Vec3};

fn sample(note: Option<&str>) -> Sensor {
    // An IDL map is the sequence of `MapEntry` XTypes 1.3 §7.5.1.3.6 binds
    // it to; `zerodds-idlc --rust-map btree-map` would make it a `BTreeMap`.
    let calibration = vec![
        zerodds_wire::MapEntry::new(1u32, Vec3 { x: 1.0, y: 0.0, z: 0.0 }),
        zerodds_wire::MapEntry::new(2u32, Vec3 { x: 0.0, y: 1.0, z: 0.0 }),
    ];
    Sensor {
        id: 7,
        name: "lidar-front".to_string(),
        mode: SensorMode::ACTIVE,
        origin: Vec3 { x: 0.1, y: -0.2, z: 1.5 },
        // a sequence mixing all three union arms
        readings: vec![
            Reading::Pose(Vec3 { x: 1.0, y: 2.0, z: 3.0 }),
            Reading::Scalar(42.5),
            // The default case carries the discriminator it went out under;
            // 3 is no label of the union.
            Reading::Raw(3, 0xAB),
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
                <Sensor as DdsType>::encode(s, &mut b).expect("xcdr2 encode");
                b
            },
            |b| <Sensor as DdsType>::decode(b).expect("xcdr2 decode"),
        );
        check(
            "XCDR1",
            &s,
            |s| {
                let mut b = Vec::new();
                <Sensor as DdsType>::encode_xcdr1(s, &mut b).expect("xcdr1 encode");
                b
            },
            |b| <Sensor as DdsType>::decode_xcdr1(b).expect("xcdr1 decode"),
        );
    }
    println!("\nall complex-type round-trips pass under both XCDR2 and XCDR1 ✓");
}
