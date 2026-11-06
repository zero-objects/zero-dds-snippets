// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 ZeroDDS Contributors
//! `zerodds-build`'s `rust_type` (GitHub #33): a sample written from
//! checked Rust types is the octets of its IDL types, and a sample whose
//! IDL value the Rust type does not accept is rejected with the reason.

mod dtg;

#[allow(dead_code, missing_docs, non_snake_case, unused_imports)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/track.rs"));
}

use core::num::NonZeroU32;

use generated::app::Track;
use zerodds_dcps::DdsType;
use zerodds_wire::{Endianness, Reader, Writer};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let track = Track {
        id: NonZeroU32::new(7).ok_or("seven is not zero")?,
        at: "051430ZNOV26".parse()?,
        seen: vec!["051400ZNOV26".parse()?, "051415ZNOV26".parse()?],
        callsign: String::from("ALPHA1"),
    };

    // XCDR2 little endian, the octets a C or C++ peer built from
    // `idl/track.idl` reads into `uint32_t` and `char *`.
    let mut bytes = Vec::new();
    <Track as DdsType>::encode(&track, &mut bytes)?;
    println!("{} octets: {}", bytes.len(), hex(&bytes));
    assert_eq!(<Track as DdsType>::decode(&bytes)?, track);
    println!("read back: {} at {}", track.id, track.at);

    // A peer that holds the IDL `string` writes a text that is no
    // date-time group. The decoder hands out no `Track` and says why.
    let mut peer = Writer::with_max_align(Endianness::Little, 4);
    peer.write_u32(7);
    peer.write_string("noon");
    empty_sequence_of_strings(&mut peer);
    peer.write_string("BRAVO2");
    let peer = peer.finish();
    let mut reader = Reader::with_max_align(&peer, Endianness::Little, 4);
    match Track::decode(&mut reader) {
        Ok(track) => return Err(format!("a sample with `noon` was read: {track:?}").into()),
        Err(refused) => println!("rejected: {refused}"),
    }

    // A key of zero is refused the same way: `NonZeroU32` holds none.
    let mut zero = Writer::with_max_align(Endianness::Little, 4);
    zero.write_u32(0);
    zero.write_string("051430ZNOV26");
    empty_sequence_of_strings(&mut zero);
    zero.write_string("CHARLIE3");
    let zero = zero.finish();
    assert!(<Track as DdsType>::decode(&zero).is_err());
    let mut reader = Reader::with_max_align(&zero, Endianness::Little, 4);
    match Track::decode(&mut reader) {
        Ok(track) => return Err(format!("a sample with key 0 was read: {track:?}").into()),
        Err(refused) => println!("rejected: {refused}"),
    }
    Ok(())
}

/// An empty `sequence<Dtg, 8>`: XTypes 1.3 §7.4.3.5.3 rule (12) puts a
/// DHEADER in front of a sequence of strings, then the element count.
fn empty_sequence_of_strings(writer: &mut Writer) {
    let header = writer.write_dheader();
    writer.write_u32(0);
    writer.finish_dheader(header);
}

fn hex(bytes: &[u8]) -> String {
    bytes.iter().map(|byte| format!("{byte:02x}")).collect()
}
