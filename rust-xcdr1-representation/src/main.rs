//! Choosing the **data representation** — XCDR2 (the DDS default) vs XCDR1
//! (classic CDR) — for a typed sample, and seeing the wire difference.
//!
//! `Reading` is generated from `demo.idl` by `zerodds-build` (see `build.rs`).
//! It carries two codec pairs as inherent functions, `encode`/`decode` and
//! `encode_xcdr1`/`decode_xcdr1`. Both write the same member sequence; what
//! separates the representations is the alignment cap the `Writer` and
//! `Reader` were constructed with, XTypes 1.3 §7.4.1:
//!
//! * **XCDR2** (OMG XTypes 1.3 — the ZeroDDS / DDS default):
//!   `MAXALIGN(VERSION2) = 4`, so 8-byte primitives cap at a 4-byte boundary.
//! * **XCDR1** (classic CDR — `DDS_CDR`, the pre-XTypes wire many legacy /
//!   Fast-DDS-classic peers speak): `MAXALIGN(VERSION1) = 8`, natural
//!   8-byte alignment.
//!
//! `Reading` is `@final`, so neither representation carries a DHEADER — the
//! only thing that changes is the alignment of the 8-byte `value` after the
//! 4-byte `seq`. (`@appendable`/`@mutable` add a DHEADER / a PL_CDR1
//! parameter list that also differ between the representations; a generated
//! type's `DdsType` impl frames those, and ZeroDDS implements the complete
//! XCDR1 wire in every language binding.)

use zerodds_wire::{Endianness, Reader, Writer};

// The build script writes the generated module here (`demo.idl` ->
// `demo.rs`). Wrapped in a module with the lint allows as outer
// attributes, so the generated file's own imports don't clash with ours.
#[allow(non_snake_case, unused_imports, dead_code)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/demo.rs"));
}
use generated::Reading;

/// The alignment cap XTypes 1.3 §7.4.1 fixes for each representation.
const MAXALIGN_XCDR2: usize = 4;
const MAXALIGN_XCDR1: usize = 8;

/// Encodes `v` little-endian as XCDR2 (`MAXALIGN` 4).
fn encode_xcdr2(v: &Reading) -> Vec<u8> {
    let mut w = Writer::with_max_align(Endianness::Little, MAXALIGN_XCDR2);
    v.encode(&mut w).expect("xcdr2 encode");
    w.finish()
}

/// Encodes `v` little-endian as XCDR1 (`MAXALIGN` 8).
fn encode_xcdr1(v: &Reading) -> Vec<u8> {
    let mut w = Writer::with_max_align(Endianness::Little, MAXALIGN_XCDR1);
    v.encode_xcdr1(&mut w).expect("xcdr1 encode");
    w.finish()
}

/// Decodes little-endian XCDR2 bytes.
fn decode_xcdr2(bytes: &[u8]) -> Reading {
    let mut r = Reader::with_max_align(bytes, Endianness::Little, MAXALIGN_XCDR2);
    Reading::decode(&mut r).expect("xcdr2 decode")
}

/// Decodes little-endian XCDR1 bytes.
fn decode_xcdr1(bytes: &[u8]) -> Reading {
    let mut r = Reader::with_max_align(bytes, Endianness::Little, MAXALIGN_XCDR1);
    Reading::decode_xcdr1(&mut r).expect("xcdr1 decode")
}

fn hex(bytes: &[u8]) -> String {
    bytes
        .iter()
        .map(|b| format!("{b:02x}"))
        .collect::<Vec<_>>()
        .join(" ")
}

fn main() {
    let sample = Reading {
        seq: 7,
        value: 2.5,
        label: "ok".to_string(),
    };
    println!("sample: Reading {{ seq: 7, value: 2.5, label: \"ok\" }}\n");

    let x2 = encode_xcdr2(&sample);
    println!("XCDR2 ({} bytes): {}", x2.len(), hex(&x2));

    let x1 = encode_xcdr1(&sample);
    println!("XCDR1 ({} bytes): {}", x1.len(), hex(&x1));

    assert_ne!(x2, x1, "the two representations must differ for this type");

    // Each stream round-trips when read back with the SAME representation; a
    // reader and writer must agree on it (DDS negotiates the representation via
    // the DataRepresentation QoS + the RTPS encapsulation id: `CDR_LE` 0x0001
    // for XCDR1, `D_CDR2_LE` 0x0009 for XCDR2).
    let b2 = decode_xcdr2(&x2);
    let b1 = decode_xcdr1(&x1);
    assert_eq!(
        (b2.seq, b2.value, b2.label.as_str()),
        (7, 2.5, "ok"),
        "XCDR2 round-trip"
    );
    assert_eq!(
        (b1.seq, b1.value, b1.label.as_str()),
        (7, 2.5, "ok"),
        "XCDR1 round-trip"
    );

    println!("\nboth round-trip cleanly with their matching representation");
    println!(
        "XCDR1 is {} bytes longer here (the 8-byte alignment pad before `value`).",
        x1.len().saturating_sub(x2.len())
    );
}
