//! Endianness × XCDR-version wire-proof harness (ZeroDDS Rust reference PSM).
//!
//! Encodes every CANONICAL.md sample of `features.idl` in FOUR representations
//! and writes one golden per (feature, repr):
//!
//!   * XCDR2-LE  — PLAIN_CDR2, little-endian (max-align 4, DHEADER present)
//!   * XCDR2-BE  — PLAIN_CDR2, big-endian
//!   * XCDR1-LE  — PLAIN_CDR  (max-align 8, NO DHEADER)
//!   * XCDR1-BE  — PLAIN_CDR, big-endian
//!
//! The bytes written are the *bare CDR body* (no 4-byte encapsulation header);
//! the vendor oracle (FastCDR) strips its encapsulation before comparison.
//!
//! All four are produced through the SAME generated codec the ZeroDDS
//! DataWriter uses: `zerodds_dcps::DdsType` names exactly these four encode
//! pairs — `encode`/`decode` (XCDR2-LE), `encode_be`/`decode_be` (XCDR2-BE),
//! `encode_xcdr1`/`decode_xcdr1` (XCDR1-LE) and `encode_xcdr1_be`/
//! `decode_xcdr1_be` (XCDR1-BE) — each building its own
//! `zerodds_wire::Writer` with that representation's endianness and alignment
//! cap. The harness picks the pair and reads the bytes back.
//!
//! ## This harness does not build against rc.7
//!
//! `zerodds-build` writes the XCDR1 pair only under `dds_type_impls(true)`,
//! and that option also writes `impl zerodds_dcps::DdsType for feat::Perm`.
//! A bitmask is generated as `pub type Perm = u32;`, so that impl is a foreign
//! trait on a primitive — `error[E0117]`. Without the option there is no
//! XCDR1 codec at all and two of the four representations cannot be produced,
//! so the corpus's `bitmask Perm` blocks the harness either way.
//!
//! Modes:
//!   ENCODE     — write all 24 goldens, print hex+len+sha256.
//!   ROUNDTRIP  — encode then decode-back each repr, assert == canonical
//!                (self-consistency of the ZeroDDS codec across all 4 reprs).

use std::process::ExitCode;

use sha2::{Digest, Sha256};
use zerodds_dcps::DdsType;

// `zerodds-build` writes the module into OUT_DIR; the lint allows sit on the
// wrapping module because `include!` splices the tokens into this file.
#[allow(non_snake_case, non_camel_case_types, unused_imports, dead_code, clippy::all)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/features.rs"));
}

use generated::feat;
use generated::feat::{Arr, Bits, Flags, Hue, MapEnum, Mut, Prim, Pt, Sel, Tree, WStr};

const DIR: &str = concat!(env!("CARGO_MANIFEST_DIR"), "/../goldens");

/// The four representations, by the `DdsType` method pair that writes and
/// reads each. The alignment cap is what separates the two XCDR versions —
/// PLAIN_CDR2 caps at four octets, classic PLAIN_CDR aligns naturally up to
/// eight — and each method builds its own writer with it.
const TAGS: [&str; 4] = ["xcdr2-le", "xcdr2-be", "xcdr1-le", "xcdr1-be"];

/// Encode `$sample` in the representation `$tag` names.
macro_rules! encode_repr {
    ($sample:expr, $tag:expr, $out:expr) => {
        match $tag {
            "xcdr2-le" => DdsType::encode($sample, $out),
            "xcdr2-be" => DdsType::encode_be($sample, $out),
            "xcdr1-le" => DdsType::encode_xcdr1($sample, $out),
            "xcdr1-be" => DdsType::encode_xcdr1_be($sample, $out),
            other => panic!("no such representation: {other}"),
        }
    };
}

/// Read `$bytes` back in the representation `$tag` names.
macro_rules! decode_repr {
    ($ty:ty, $tag:expr, $bytes:expr) => {
        match $tag {
            "xcdr2-le" => <$ty as DdsType>::decode($bytes),
            "xcdr2-be" => <$ty as DdsType>::decode_be($bytes),
            "xcdr1-le" => <$ty as DdsType>::decode_xcdr1($bytes),
            "xcdr1-be" => <$ty as DdsType>::decode_xcdr1_be($bytes),
            other => panic!("no such representation: {other}"),
        }
    };
}

/// `$bytes` read back as the type of `like`, in the representation `tag`
/// names. A function so the type is `like`'s; `decode_repr!(_, ..)` left it
/// to inference, which the `DdsType` impls of the generated module and of
/// `zerodds_dcps` itself leave ambiguous.
fn decode_like<T: DdsType>(
    _like: &T,
    tag: &str,
    bytes: &[u8],
) -> Result<T, zerodds_dcps::DecodeError> {
    decode_repr!(T, tag, bytes)
}

// ---- canonical samples (identical to _interop/CANONICAL.md) ----------------

fn canonical_wstr() -> WStr {
    WStr {
        // A `wstring` member is a plain `String` in the generated type; the
        // UTF-16 framing is the codec's, not the value's.
        label: "café".to_string(),
        text: "日本語\u{1F389}".to_string(),
    }
}
fn canonical_mut() -> Mut {
    Mut { a: 1_000_000, b: 2.5, c: "ok".to_string() }
}
fn canonical_bits() -> Bits {
    // A bitset is materialised as a struct with one field per named bitfield,
    // and a bitmask as module-level `u32` constants.
    let flags = Flags { kind: 5, prio: 20 };
    Bits { perm: feat::READ | feat::EXEC, flags }
}
fn canonical_tree() -> Tree {
    Tree {
        value: 1,
        kids: vec![
            Tree { value: 2, kids: vec![Tree { value: 4, kids: vec![] }] },
            Tree { value: 3, kids: vec![] },
        ],
    }
}
fn canonical_arr() -> Arr {
    Arr { grid: [[10, 11, 12], [13, 14, 15]], shape: [Pt { x: 1, y: 2 }, Pt { x: 3, y: 4 }] }
}
fn canonical_prim() -> Prim {
    Prim {
        i8: -128, u8: 255, i16: -32768, u16: 65535, i32: -2_147_483_648, u32: 4_294_967_295,
        i64: -9_223_372_036_854_775_808, u64: 18_446_744_073_709_551_615,
        f32: 3.5, f64: -1234.5, b: true, o: 0xAB, ch: 'Z',
    }
}

fn canonical_mapenum() -> MapEnum {
    let mut m = ::std::collections::BTreeMap::new();
    m.insert(3, Pt { x: 11, y: 12 });
    MapEnum { h: Hue::H_BLUE, m, sels: vec![Sel::N(9)] }
}

fn sha256_hex(bytes: &[u8]) -> String {
    let mut h = Sha256::new();
    h.update(bytes);
    h.finalize().iter().map(|b| format!("{b:02x}")).collect()
}

fn hexline(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{b:02x}")).collect::<Vec<_>>().join(" ")
}

/// Encode one (feature, repr), write golden, print metadata.
///
/// A macro and not a generic function: the generated codec is a pair of
/// *inherent* method pairs, not a trait, so there is no bound to write.
macro_rules! encode_one {
    ($name:expr, $sample:expr) => {{
    let name: &str = $name;
    let sample = &$sample;
    let mut ok = true;
    for tag in TAGS {
        let mut bytes = Vec::new();
        if let Err(e) = encode_repr!(sample, tag, &mut bytes) {
            eprintln!("FAIL encode {name}.{tag}: {e:?}");
            ok = false;
            continue;
        }
        let path = format!("{DIR}/{name}.{tag}.rust.bin");
        if let Err(e) = std::fs::write(&path, &bytes) {
            eprintln!("FAIL write {path}: {e}");
            ok = false;
            continue;
        }
        println!(
            "ENCODE {name:<5} {tag:<8} len={len:<3} sha256={sha}",
            len = bytes.len(), sha = sha256_hex(&bytes)
        );
        println!("  hex: {}", hexline(&bytes));
    }
    ok
    }};
}

/// Encode then decode-back each repr; assert equal to the canonical sample.
macro_rules! roundtrip_one {
    ($name:expr, $sample:expr) => {{
    let name: &str = $name;
    let sample = &$sample;
    let mut ok = true;
    for tag in TAGS {
        let mut bytes = Vec::new();
        if let Err(e) = encode_repr!(sample, tag, &mut bytes) {
            eprintln!("FAIL encode {name}.{tag}: {e:?}");
            ok = false;
            continue;
        }
        match decode_like(sample, tag, &bytes) {
            Ok(back) if &back == sample => {
                println!("RT   {name:<5} {tag:<8} OK ({} bytes)", bytes.len());
            }
            Ok(back) => {
                eprintln!("FAIL roundtrip {name}.{tag}: decoded != canonical");
                eprintln!("  want {sample:?}\n  got  {back:?}");
                ok = false;
            }
            Err(e) => {
                eprintln!("FAIL decode {name}.{tag}: {e:?}");
                ok = false;
            }
        }
    }
    ok
    }};
}

macro_rules! for_all {
    ($f:ident) => {{
        let mut ok = true;
        ok &= $f!("wstr", canonical_wstr());
        ok &= $f!("mut", canonical_mut());
        ok &= $f!("bits", canonical_bits());
        ok &= $f!("tree", canonical_tree());
        ok &= $f!("arr", canonical_arr());
        ok &= $f!("prim", canonical_prim());
        ok &= $f!("mapenum", canonical_mapenum());
        ok
    }};
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Every sample round-trips through every one of the 4 reprs — proves the
    /// ZeroDDS codec is self-consistent across endianness AND XCDR version.
    #[test]
    fn all_reprs_roundtrip() {
        assert!(for_all!(roundtrip_one));
    }
}

fn main() -> ExitCode {
    let args: Vec<String> = std::env::args().collect();
    let ok = match args.get(1).map(String::as_str) {
        Some("ENCODE") => for_all!(encode_one),
        Some("ROUNDTRIP") => for_all!(roundtrip_one),
        None => for_all!(encode_one) && for_all!(roundtrip_one),
        other => {
            eprintln!("usage: endian_proof [ENCODE|ROUNDTRIP]  (got {other:?})");
            return ExitCode::FAILURE;
        }
    };
    if ok { ExitCode::SUCCESS } else { ExitCode::FAILURE }
}
