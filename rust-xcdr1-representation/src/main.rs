//! Choosing the **data representation** — XCDR2 (the DDS default) vs XCDR1
//! (classic CDR) — for a typed sample, and seeing the wire difference.
//!
//! `Reading` is generated from `demo.idl` by the ZeroDDS IDL-to-Rust backend
//! (see `build.rs`, `cdr_only` mode). It implements the low-level
//! `CdrEncode`/`CdrDecode`; the `BufferWriter`'s *max alignment* selects the
//! representation:
//!
//! * **XCDR2** (OMG XTypes 1.3 — the ZeroDDS / DDS default): 8-byte primitives
//!   capped to a 4-byte alignment (`with_max_alignment(4)`).
//! * **XCDR1** (classic CDR — `DDS_CDR`, the pre-XTypes wire many legacy /
//!   FastDDS-classic peers speak): natural 8-byte alignment
//!   (`with_max_alignment(8)`).
//!
//! `Reading` is `@final`, so neither representation carries a DHEADER — the
//! only thing that changes is the alignment of the 8-byte `value` after the
//! 4-byte `seq`. (`@appendable`/`@mutable` add a DHEADER / PL_CDR1 that also
//! differ between the reprs; the full `DdsType::encode_xcdr1` handles those,
//! and ZeroDDS implements the complete XCDR1 wire in every language binding.)

use zerodds_cdr::{BufferReader, BufferWriter, CdrDecode, CdrEncode, Endianness};

// The build script writes the generated `cdr_only` binding here (its `#![...]`
// inner attrs stripped). Wrapped in a module so its own `use zerodds_cdr::...`
// imports don't clash with ours.
#[allow(non_snake_case, unused_imports)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/reading.rs"));
}
use generated::Reading;

/// Encodes `v` little-endian at the given max alignment (4 = XCDR2, 8 = XCDR1).
fn encode(v: &Reading, max_align: usize) -> Vec<u8> {
    let mut w = BufferWriter::new(Endianness::Little).with_max_alignment(max_align);
    v.encode(&mut w).expect("encode");
    w.into_bytes()
}

/// Decodes little-endian bytes at the given max alignment.
fn decode(bytes: &[u8], max_align: usize) -> Reading {
    let mut r = BufferReader::new(bytes, Endianness::Little).with_max_alignment(max_align);
    Reading::decode(&mut r).expect("decode")
}

fn hex(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{b:02x}")).collect::<Vec<_>>().join(" ")
}

fn main() {
    let sample = Reading { seq: 7, value: 2.5, label: "ok".to_string() };
    println!("sample: Reading {{ seq: 7, value: 2.5, label: \"ok\" }}\n");

    let x2 = encode(&sample, 4);
    println!("XCDR2 ({} bytes): {}", x2.len(), hex(&x2));

    let x1 = encode(&sample, 8);
    println!("XCDR1 ({} bytes): {}", x1.len(), hex(&x1));

    assert_ne!(x2, x1, "the two representations must differ for this type");

    // Each stream round-trips when read back with the SAME representation; a
    // reader and writer must agree on it (DDS negotiates the representation via
    // the DataRepresentation QoS + the RTPS encapsulation id).
    let b2 = decode(&x2, 4);
    let b1 = decode(&x1, 8);
    assert_eq!((b2.seq, b2.value, b2.label.as_str()), (7, 2.5, "ok"), "XCDR2 round-trip");
    assert_eq!((b1.seq, b1.value, b1.label.as_str()), (7, 2.5, "ok"), "XCDR1 round-trip");

    println!("\nboth round-trip cleanly with their matching representation ✓");
    println!(
        "XCDR1 is {} bytes longer here (the 8-byte alignment pad before `value`).",
        x1.len().saturating_sub(x2.len())
    );
}
