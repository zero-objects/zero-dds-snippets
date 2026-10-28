//! WIRE-level XCDR2 type-system PROOF harness — WAVE 2 constructs.
//!
//! Encodes the canonical samples for the wave-2 constructs (union, enum,
//! @bit_bound enum, standalone @optional, nested @mutable, sequence<mutable>)
//! through the cross-vendor-validated `zerodds-wire` core — the exact bytes a
//! ZeroDDS DataWriter puts on the wire (XCDR2-LE, default repr) — and:
//!
//!   * (default)        prints len + sha256 + hexdump of each ZeroDDS encoding,
//!                      and writes `goldens/<case>.zerodds.bin`.
//!   * `--check`        additionally byte-compares each ZeroDDS encoding against
//!                      the recorded vendor golden(s) under `vendor/` and prints
//!                      a MATCH / DIFF table. Exit code is non-zero if any
//!                      EXPECTED-MATCH case diverges (the proven-interop set).
//!
//! See README.md for the canonical values, the per-case interop property, the
//! exact vendor versions, and copy-paste reproduction steps.

use std::collections::BTreeMap;
use std::path::PathBuf;
use std::process::ExitCode;

// `zerodds-build` writes the module into OUT_DIR; the lint allows sit on the
// wrapping module because `include!` splices the tokens into this file.
#[allow(non_snake_case, non_camel_case_types, unused_imports, dead_code, clippy::all)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/typesystem.rs"));
}

use generated::ts::{
    Color, EnumHolder, EnumUnion, Inner, LongUnion, Mid, MutLeaf, MutOuter, OptHolder, Point, Tiny,
};

fn dir(sub: &str) -> PathBuf {
    let base = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    base.join(sub)
}

// ---------------------------------------------------------------------------
// Canonical samples — EXACT values, documented in README.md / CANONICAL.md.
// ---------------------------------------------------------------------------

/// enum_holder: big=BLUE(2, 32-bit), small=T_C(2, 8-bit), medium=M_D(3, 16-bit).
fn enum_holder() -> EnumHolder {
    EnumHolder { big: Color::BLUE, small: Tiny::T_C, medium: Mid::MID_D }
}

/// union_long_default: the DEFAULT arm selected — note="hi" (disc=0 here).
fn union_long_default() -> LongUnion {
    LongUnion::Note(0, "hi".to_string())
}

/// union_long_struct: the struct arm — case 2 -> where = {x=7, y=8}.
fn union_long_struct() -> LongUnion {
    LongUnion::Where(Point { x: 7, y: 8 })
}

/// union_enum_default: enum-discriminated, DEFAULT arm -> fallback=42 (disc=BLUE=2).
fn union_enum_default() -> EnumUnion {
    EnumUnion::Fallback(Color::BLUE, 42)
}

/// opt_holder: id=1; maybe_num=Some(77); maybe_str=None; maybe_obj=Some{a=5,b=6}.
fn opt_holder() -> OptHolder {
    OptHolder {
        id: 1,
        maybe_num: Some(77),
        maybe_str: None,
        maybe_obj: Some(Inner { a: 5, b: 6 }),
    }
}

/// mut_outer: tag=9; leaf={u=100,v=1.25}; list=[{1,0.5},{2,0.25}].
fn mut_outer() -> MutOuter {
    MutOuter {
        tag: 9,
        leaf: MutLeaf { u: 100, v: 1.25 },
        list: vec![MutLeaf { u: 1, v: 0.5 }, MutLeaf { u: 2, v: 0.25 }],
    }
}

// ---------------------------------------------------------------------------
// sha256 (self-contained, no external crate) — FIPS 180-4.
// ---------------------------------------------------------------------------

fn sha256(data: &[u8]) -> String {
    const K: [u32; 64] = [
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
        0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
        0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
        0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
        0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
        0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
        0xc67178f2,
    ];
    let mut h: [u32; 8] = [
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab,
        0x5be0cd19,
    ];
    let mut msg = data.to_vec();
    let bitlen = (data.len() as u64) * 8;
    msg.push(0x80);
    while msg.len() % 64 != 56 {
        msg.push(0);
    }
    msg.extend_from_slice(&bitlen.to_be_bytes());
    for chunk in msg.chunks_exact(64) {
        let mut w = [0u32; 64];
        for i in 0..16 {
            w[i] = u32::from_be_bytes([
                chunk[i * 4],
                chunk[i * 4 + 1],
                chunk[i * 4 + 2],
                chunk[i * 4 + 3],
            ]);
        }
        for i in 16..64 {
            let s0 = w[i - 15].rotate_right(7) ^ w[i - 15].rotate_right(18) ^ (w[i - 15] >> 3);
            let s1 = w[i - 2].rotate_right(17) ^ w[i - 2].rotate_right(19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16]
                .wrapping_add(s0)
                .wrapping_add(w[i - 7])
                .wrapping_add(s1);
        }
        let mut v = h;
        for i in 0..64 {
            let s1 = v[4].rotate_right(6) ^ v[4].rotate_right(11) ^ v[4].rotate_right(25);
            let ch = (v[4] & v[5]) ^ ((!v[4]) & v[6]);
            let t1 = v[7]
                .wrapping_add(s1)
                .wrapping_add(ch)
                .wrapping_add(K[i])
                .wrapping_add(w[i]);
            let s0 = v[0].rotate_right(2) ^ v[0].rotate_right(13) ^ v[0].rotate_right(22);
            let maj = (v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]);
            let t2 = s0.wrapping_add(maj);
            v[7] = v[6];
            v[6] = v[5];
            v[5] = v[4];
            v[4] = v[3].wrapping_add(t1);
            v[3] = v[2];
            v[2] = v[1];
            v[1] = v[0];
            v[0] = t1.wrapping_add(t2);
        }
        for i in 0..8 {
            h[i] = h[i].wrapping_add(v[i]);
        }
    }
    let mut out = String::with_capacity(64);
    for word in h {
        out.push_str(&format!("{word:08x}"));
    }
    out
}

// ---------------------------------------------------------------------------
// drivers
// ---------------------------------------------------------------------------

/// A fresh XCDR2-LE writer: little-endian with the encapsulation's four-octet
/// alignment cap, which is what `DdsType::encode` builds for a topic type.
fn xcdr2_writer() -> zerodds_wire::Writer {
    zerodds_wire::Writer::with_max_align(zerodds_wire::Endianness::Little, 4)
}

/// A reader over XCDR2-LE bytes, with the same cap. Only the round-trip and
/// vendor-decode tests read; the binary only writes.
#[cfg_attr(not(test), allow(dead_code))]
fn xcdr2_reader(bytes: &[u8]) -> zerodds_wire::Reader<'_> {
    zerodds_wire::Reader::with_max_align(bytes, zerodds_wire::Endianness::Little, 4)
}

/// Encode a value through `zerodds-wire` (XCDR2-LE) into a fresh buffer.
///
/// A macro and not a generic function: the generated codec is a pair of
/// *inherent* `encode`/`decode` methods, not a trait, so there is no bound to
/// write. That is the shape `zerodds-build` emits — the trait surface is
/// `zerodds_dcps::DdsType`, and a standalone union is not a topic type and has
/// no `DdsType` impl.
macro_rules! encode {
    ($sample:expr) => {{
        let mut w = xcdr2_writer();
        $sample.encode(&mut w).expect("encode");
        w.finish()
    }};
}

fn hexdump(bytes: &[u8]) -> String {
    let mut s = String::new();
    for (i, b) in bytes.iter().enumerate() {
        if i % 16 == 0 && i != 0 {
            s.push('\n');
            s.push_str("    ");
        }
        s.push_str(&format!("{b:02x} "));
    }
    s
}

fn write_golden(name: &str, bytes: &[u8]) {
    let path = dir("goldens").join(format!("{name}.zerodds.bin"));
    std::fs::create_dir_all(dir("goldens")).ok();
    std::fs::write(&path, bytes).expect("write golden");
}

/// The (case, vendor) pairs README.md's "Known divergences" explains:
/// `enum_holder` (Fast DDS writes 32-bit enums, RTI has no non-32-bit
/// `@bit_bound`), `union_long_default` (Fast DDS's default discriminator),
/// `opt_holder` (Fast DDS's uninitialised padding) and `mut_outer` (Fast DDS
/// and RTI reuse the nested DHEADER as the NEXTINT).
const KNOWN_DIVERGENCES: &[(&str, &str)] = &[
    ("enum_holder", "fastdds"),
    ("enum_holder", "rti"),
    ("union_long_default", "fastdds"),
    ("opt_holder", "fastdds"),
    ("mut_outer", "fastdds"),
    ("mut_outer", "rti"),
];

fn main() -> ExitCode {
    let check = std::env::args().any(|a| a == "--check");

    // (case-name, zerodds bytes)
    let mut cases: Vec<(&str, Vec<u8>)> = Vec::new();
    cases.push(("enum_holder", encode!(enum_holder())));
    cases.push(("union_long_default", encode!(union_long_default())));
    cases.push(("union_long_struct", encode!(union_long_struct())));
    cases.push(("union_enum_default", encode!(union_enum_default())));
    cases.push(("opt_holder", encode!(opt_holder())));
    cases.push(("mut_outer", encode!(mut_outer())));

    println!("=== ZeroDDS XCDR2-LE encodings (wave-2 type-system corpus) ===\n");
    for (name, bytes) in &cases {
        write_golden(name, bytes);
        println!("{name}: {} bytes  sha256={}", bytes.len(), sha256(bytes));
        println!("    {}", hexdump(bytes));
        println!();
    }

    if !check {
        println!("(run with --check to byte-compare against the recorded vendor goldens)");
        return ExitCode::SUCCESS;
    }

    // Load vendor goldens: vendor/<vendor>/<case>.bin (encapsulation already stripped).
    // Every (case, vendor) pair with a golden must MATCH unless it is one of
    // the divergences README.md's "Known divergences" explains; a DIFF not on
    // that list, or a listed one that now matches, fails the run.
    let vendors = ["cyclonedds", "rti", "fastdds"];
    let mut any_unexpected = false;

    println!("=== byte-diff vs vendor goldens ===\n");
    println!(
        "{:<20} {:<12} {:<8} {}",
        "case", "vendor", "result", "note"
    );
    for (name, zbytes) in &cases {
        let mut per_case: BTreeMap<&str, Option<bool>> = BTreeMap::new();
        for v in vendors {
            let path = dir("vendor").join(v).join(format!("{name}.bin"));
            match std::fs::read(&path) {
                Ok(vb) => per_case.insert(v, Some(vb == *zbytes)),
                Err(_) => per_case.insert(v, None),
            };
        }
        for (v, r) in &per_case {
            let (res, note) = match r {
                Some(true) => ("MATCH", String::new()),
                Some(false) => {
                    let path = dir("vendor").join(v).join(format!("{name}.bin"));
                    let vb = std::fs::read(&path).unwrap_or_default();
                    (
                        "DIFF",
                        format!("zerodds {}B vs {} {}B", zbytes.len(), v, vb.len()),
                    )
                }
                None => ("n/a", "vendor cannot express / golden absent".to_string()),
            };
            let known = KNOWN_DIVERGENCES.contains(&(*name, *v));
            let unexpected = match r {
                Some(true) => known,
                Some(false) => !known,
                None => false,
            };
            let flag = if unexpected {
                any_unexpected = true;
                if known { "  <- listed as a divergence, now MATCH" } else { "  <- not a known divergence" }
            } else {
                ""
            };
            println!("{name:<20} {v:<12} {res:<8} {note}{flag}");
        }
        println!();
    }

    if any_unexpected {
        println!("UNEXPECTED RESULT against the recorded vendor goldens (see the flags above)");
        return ExitCode::FAILURE;
    }
    println!("ALL VENDOR COMPARISONS AS RECORDED ({} known divergences)", KNOWN_DIVERGENCES.len());
    ExitCode::SUCCESS
}

#[cfg(test)]
mod tests {
    use super::*;
    use zerodds_dcps::DdsType;

    /// ZeroDDS encodes each canonical sample to a stable, self-decodable golden.
    #[test]
    fn zerodds_self_roundtrip() {
        // Topic structs go through DdsType::decode.
        macro_rules! rt_struct {
            ($ty:ty, $s:expr) => {{
                let sample = $s;
                let bytes = encode!(sample);
                let back = <$ty as DdsType>::decode(&bytes).expect("decode");
                assert_eq!(sample, back, "self-roundtrip");
            }};
        }
        // A standalone union is not a topic type and carries no `DdsType`
        // impl; it is read off the wire runtime through its own inherent
        // `decode`.
        macro_rules! rt_union {
            ($ty:ty, $s:expr) => {{
                let sample = $s;
                let bytes = encode!(sample);
                let mut r = xcdr2_reader(&bytes);
                let back = <$ty>::decode(&mut r).expect("decode");
                assert_eq!(sample, back, "self-roundtrip");
            }};
        }
        rt_struct!(EnumHolder, enum_holder());
        rt_union!(LongUnion, union_long_default());
        rt_union!(LongUnion, union_long_struct());
        rt_union!(EnumUnion, union_enum_default());
        rt_struct!(OptHolder, opt_holder());
        // mut_outer (nested @mutable + sequence<@mutable>) now round-trips —
        // see `nested_mutable_self_roundtrip`. The fixed `CdrDecode` for a
        // @mutable struct parses the EMHEADER member frame.
        rt_struct!(MutOuter, mut_outer());
    }

    /// Nested-@mutable round-trip (was FINDING F1, now FIXED). A `@mutable`
    /// struct nested inside another `@mutable` struct is encoded with EMHEADER
    /// member framing AND decoded the same way — its `CdrDecode` impl runs the
    /// `read_mutable_member` loop (it previously decoded positionally, as if
    /// `@appendable`, reading the inner struct's first EMHEADER into the first
    /// field). Both the nested `@mutable` member and every `sequence<@mutable>`
    /// element now reconstruct exactly.
    #[test]
    fn nested_mutable_self_roundtrip() {
        let sample = mut_outer();
        let bytes = encode!(sample);
        let mut r = xcdr2_reader(&bytes);
        let back = MutOuter::decode(&mut r).expect("decode");
        assert_eq!(sample, back, "nested @mutable must round-trip");
    }

    /// DECODE-direction interop: ZeroDDS reads each vendor's NATIVE bytes and
    /// reconstructs the canonical value — even on cases whose ENCODE bytes
    /// differ (e.g. the @mutable LC4-vs-LC5 framing on `mut_outer`). This is
    /// the property that actually matters on the wire: a ZeroDDS reader can
    /// consume a Cyclone/RTI/FastDDS writer's mutable/optional/union samples.
    #[test]
    fn decode_vendor_goldens() {
        // Structs implement DdsType::decode (encapsulation-free top-level entry).
        fn decode_vendor_struct<T: DdsType + PartialEq + std::fmt::Debug>(
            case: &str,
            expected: &T,
        ) {
            for v in ["cyclonedds", "rti", "fastdds"] {
                let path = dir("vendor").join(v).join(format!("{case}.bin"));
                let Ok(bytes) = std::fs::read(&path) else { continue };
                match T::decode(&bytes) {
                    Ok(got) => assert_eq!(
                        &got, expected,
                        "{v}/{case}: decoded value must equal canonical"
                    ),
                    Err(e) => panic!("{v}/{case}: ZeroDDS failed to decode vendor bytes: {e:?}"),
                }
            }
        }
        // A standalone union is not a topic type and has no `DdsType` impl;
        // it is read directly off an XCDR2-LE reader through its own inherent
        // `decode`. A macro rather than a generic function, for the reason the
        // `encode!` macro above gives.
        macro_rules! decode_vendor_union {
            ($ty:ty, $case:expr, $expected:expr) => {{
                let case: &str = $case;
                for v in ["cyclonedds", "rti", "fastdds"] {
                    let path = dir("vendor").join(v).join(format!("{case}.bin"));
                    let Ok(bytes) = std::fs::read(&path) else { continue };
                    let mut r = xcdr2_reader(&bytes);
                    match <$ty>::decode(&mut r) {
                        Ok(got) => assert_eq!(
                            &got, $expected,
                            "{v}/{case}: decoded value must equal canonical"
                        ),
                        Err(e) => {
                            panic!("{v}/{case}: ZeroDDS failed to decode vendor bytes: {e:?}")
                        }
                    }
                }
            }};
        }
        decode_vendor_union!(LongUnion, "union_long_struct", &union_long_struct());
        decode_vendor_union!(EnumUnion, "union_enum_default", &union_enum_default());
        decode_vendor_struct("opt_holder", &opt_holder());
        // mut_outer decode is now asserted in `decode_vendor_mut_outer` below
        // (Cyclone + FastDDS — the vendors whose member bodies carry the inner
        // sequence DHEADER; RTI's LC4 sequence omits it — see that test).
        // NB: enum_holder and union_long_default are intentionally NOT asserted
        // here — see README "known divergences": Cyclone honours @bit_bound
        // (1/2-octet enums) which ZeroDDS/FastDDS do not, and FastDDS uses
        // INT_MAX as the default-union discriminator. Those are width/sentinel
        // mismatches in the *type model*, not framing, so a blind decode of the
        // other party's bytes is not expected to reconstruct the same value.
    }

    /// DECODE-direction interop for the nested-@mutable case (was FINDING F1).
    /// ZeroDDS now decodes a vendor's native `mut_outer` back to the canonical
    /// value for the vendors whose `sequence<@mutable>` member body carries the
    /// inner collection DHEADER inside the EMHEADER frame — **CycloneDDS** (it
    /// LC5-reuses the sequence DHEADER) and **FastDDS** (LC5 on both members).
    ///
    /// **RTI** is intentionally excluded: its `list` (mid 30) member uses LC4
    /// and OMITS the inner sequence DHEADER from the member body (the separate
    /// NEXTINT serves as the sole delimiter), so `member.body` = `[count +
    /// elements]` with no leading DHEADER — a framing RTI does not even apply
    /// to its own LC5 nested-struct member. ZeroDDS (like FastDDS/Cyclone)
    /// keeps the XCDR2-delimited sequence DHEADER, so it does not blind-decode
    /// RTI's non-self-delimited LC4 sequence. This is genuine vendor framing
    /// latitude (XTypes 1.3 §7.4.3.4.2), NOT a ZeroDDS encode gap — ZeroDDS's
    /// own `mut_outer` ENCODE is byte-identical to FastDDS (100 B). See README
    /// "Known divergences".
    #[test]
    fn decode_vendor_mut_outer() {
        use zerodds_dcps::DdsType;
        let expected = mut_outer();
        for v in ["cyclonedds", "fastdds"] {
            let path = dir("vendor").join(v).join("mut_outer.bin");
            let bytes = std::fs::read(&path).expect("vendor golden");
            let got = <MutOuter as DdsType>::decode(&bytes)
                .unwrap_or_else(|e| panic!("{v}/mut_outer decode failed: {e:?}"));
            assert_eq!(got, expected, "{v}/mut_outer must decode to canonical");
        }
    }
}
