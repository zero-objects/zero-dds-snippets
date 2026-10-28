// The website's docs/flatdata-derive.html usage snippet, made runnable.
// The derive writes the `FlatStruct` impl the hand-written example spells out;
// the page states the TYPE_HASH is the first 16 bytes of a SHA-256 over the
// structural signature, so the demo prints it rather than asserting a constant
// the page does not name.
use zerodds_flatdata::FlatStruct;
use zerodds_flatdata_derive::FlatStruct;

#[derive(Copy, Clone, FlatStruct)]
#[repr(C)]
struct Pose {
    x: f64,
    y: f64,
    z: f64,
}

fn main() {
    assert_eq!(Pose::WIRE_SIZE, core::mem::size_of::<Pose>());
    assert_ne!(Pose::TYPE_HASH, [0u8; 16]);
    println!(
        "flatdata-derive: Pose WIRE_SIZE = {}, TYPE_HASH = {}",
        Pose::WIRE_SIZE,
        Pose::TYPE_HASH.iter().map(|b| format!("{b:02x}")).collect::<String>()
    );
}
