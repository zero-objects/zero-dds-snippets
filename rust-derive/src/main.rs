// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 ZeroDDS Contributors
//! The derive route of GitHub #33: `Track` is defined in Rust, with a
//! union from an enum with data and a generic type in it. Its IDL is
//! exported for every other peer, a sample is the octets the IDL route
//! writes for the same type, a borrowed view reads it without copying the
//! call sign, and a sample whose text is no date-time group is rejected
//! with the reason.
//!
//! `cargo run -- FILE` also writes the sample to `FILE`, for the C peer in
//! `peer/`.

mod dtg;

#[allow(dead_code, missing_docs, non_camel_case_types, non_snake_case, unused_imports)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/track.rs"));
}

use core::num::NonZeroU32;

use dtg::Dtg;
use generated::app::Track as IdlTrack;
use zerodds_codec::{DdsType, Encapsulation};

/// Two values of one type: a generic type, one IDL type per argument
/// (`Pair<f64>` is `app::Pair_double`).
#[derive(DdsType, Debug, Clone, PartialEq)]
#[dds(name = "app::Pair_{T}", extensibility = "final")]
struct Pair<T> {
    first: T,
    second: T,
}

/// Where the track was: an enum with data is an IDL `union`.
#[derive(DdsType, Debug, Clone, PartialEq)]
#[dds(name = "app::Position", extensibility = "final", discriminator = "i16")]
enum Position {
    /// `case 1: string grid;`, a grid reference.
    #[dds(case = 1, name = "grid")]
    Grid(String),
    /// `case 2: ::app::Pair_double latlon;`, degrees.
    #[dds(case = 2, name = "latlon")]
    LatLon(Pair<f64>),
}

/// A track report, defined here and not in IDL.
#[derive(DdsType, Debug, Clone, PartialEq)]
#[dds(name = "app::Track", extensibility = "final")]
struct Track {
    /// `@key unsigned long`; a key of zero does not exist.
    #[dds(key)]
    id: NonZeroU32,
    /// `string<12>`.
    #[dds(bound = 12)]
    at: Dtg,
    /// `sequence<string, 8>`.
    #[dds(bound = 8)]
    seen: Vec<Dtg>,
    /// `string<16>`.
    #[dds(bound = 16)]
    callsign: String,
    /// The union.
    position: Position,
}

/// The same IDL type read without a copy of the call sign: a borrowed
/// field is lent out of the received sample.
#[derive(DdsType, Debug, Clone, PartialEq)]
#[dds(name = "app::Track", extensibility = "final")]
struct TrackView<'a> {
    #[dds(key)]
    id: NonZeroU32,
    #[dds(bound = 12)]
    at: Dtg,
    #[dds(bound = 8)]
    seen: Vec<Dtg>,
    #[dds(bound = 16)]
    callsign: &'a str,
    position: Position,
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let idl = Track::idl()?;
    print!("exported IDL:\n{idl}");
    if idl.trim() != include_str!("../idl/track.idl").trim() {
        return Err("idl/track.idl is no longer what Track exports".into());
    }

    let track = Track {
        id: NonZeroU32::new(7).ok_or("seven is not zero")?,
        at: "051430ZNOV26".parse()?,
        seen: vec!["051400ZNOV26".parse()?, "051415ZNOV26".parse()?],
        callsign: String::from("ALPHA1"),
        position: Position::LatLon(Pair {
            first: 51.31,
            second: 9.48,
        }),
    };
    let mut ours = Vec::new();
    zerodds_codec::encode(&track, Encapsulation::Xcdr2Le, &mut ours)?;
    println!("{} octets: {}", ours.len(), hex(&ours));

    // The IDL route: the same IDL through zerodds-build, with the same
    // date-time group for `at`. Same identifier, same object, same octets.
    let idl_route = IdlTrack {
        id: track.id,
        at: track.at,
        seen: track.seen.iter().map(ToString::to_string).collect(),
        callsign: track.callsign.clone(),
        position: generated::app::Position::Latlon(generated::app::Pair_double {
            first: 51.31,
            second: 9.48,
        }),
    };
    let mut theirs = Vec::new();
    <IdlTrack as zerodds_dcps::DdsType>::encode(&idl_route, &mut theirs)?;
    if theirs != ours
        || <IdlTrack as zerodds_dcps::DdsType>::type_identifier()
            != <Track as zerodds_dcps::DdsType>::type_identifier()
        || <IdlTrack as zerodds_dcps::DdsType>::type_object()
            != <Track as zerodds_dcps::DdsType>::type_object()
    {
        return Err("the two routes disagree".into());
    }
    println!("the IDL route writes the same octets, TypeIdentifier and TypeObject");
    let back: Track = zerodds_codec::decode(&ours, Encapsulation::Xcdr2Le)?;
    println!("read back: {} at {}, {:?}", back.id, back.at, back.position);

    // The borrowed view: its call sign points into the received octets.
    let view: TrackView<'_> = zerodds_codec::decode(&ours, Encapsulation::Xcdr2Le)?;
    if !ours.as_ptr_range().contains(&view.callsign.as_ptr()) {
        return Err("the call sign was copied".into());
    }
    println!("borrowed: {} lent from the sample", view.callsign);

    // A peer that holds the IDL `string` sends a text that is no date-time
    // group. The derived type hands out no `Track` and says why.
    let mut odd = idl_route;
    odd.seen.push(String::from("noon"));
    let mut sample = Vec::new();
    <IdlTrack as zerodds_dcps::DdsType>::encode(&odd, &mut sample)?;
    match zerodds_codec::decode::<Track>(&sample, Encapsulation::Xcdr2Le) {
        Ok(track) => return Err(format!("a sample with `noon` was read: {track:?}").into()),
        Err(refused) => println!("rejected: {refused}"),
    }

    if let Some(path) = std::env::args().nth(1) {
        std::fs::write(&path, &ours)?;
        println!("sample written to {path}");
    }
    Ok(())
}

fn hex(bytes: &[u8]) -> String {
    bytes.iter().map(|byte| format!("{byte:02x}")).collect()
}
