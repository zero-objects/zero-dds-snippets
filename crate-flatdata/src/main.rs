// The website's docs/flatdata.html usage snippet, made runnable.
// A POD struct written into a shared-memory slot and read straight back out,
// with no serialization step in between.
use std::sync::Arc;
use zerodds_flatdata::{FlatReader, FlatStruct, FlatWriter, InMemorySlotAllocator};

#[derive(Copy, Clone, Debug, PartialEq)]
#[repr(C)]
struct Pose {
    x: f64,
    y: f64,
    z: f64,
}

unsafe impl FlatStruct for Pose {
    const TYPE_HASH: [u8; 16] = [0xAA; 16];
}

fn main() {
    let alloc = Arc::new(InMemorySlotAllocator::new(0, 4, Pose::WIRE_SIZE));
    let writer = FlatWriter::<Pose>::new(Arc::clone(&alloc), 0b1);
    let reader = FlatReader::<Pose>::new(Arc::clone(&alloc), 0);

    let sent = Pose { x: 1.0, y: 2.0, z: 3.0 };
    writer.write(&sent).unwrap();
    let got = reader.read().unwrap().unwrap();
    assert_eq!(got, sent);
    println!("flatdata: {} bytes per slot, roundtripped {got:?}", Pose::WIRE_SIZE);
}
