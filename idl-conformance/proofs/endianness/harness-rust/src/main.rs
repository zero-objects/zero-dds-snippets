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
//! All four are produced through the SAME generated `CdrEncode`/`CdrDecode`
//! impls the ZeroDDS DataWriter uses — `CdrEncode::encode` already branches on
//! `BufferWriter::max_alignment()` (== 4 → XCDR2 DHEADER path, == 8 → XCDR1
//! plain path) and on the writer's `Endianness`. We just construct the writer
//! in each of the four modes.
//!
//! Modes:
//!   ENCODE     — write all 24 goldens, print hex+len+sha256.
//!   ROUNDTRIP  — encode then decode-back each repr, assert == canonical
//!                (self-consistency of the ZeroDDS codec across all 4 reprs).

use std::process::ExitCode;

use sha2::{Digest, Sha256};
use zerodds_cdr::{BufferReader, BufferWriter, CdrDecode, CdrEncode, Endianness};

#[path = "../generated/features.rs"]
mod generated;

use generated::feat::{Arr, Bits, Flags, Mut, Perm, Prim, Pt, Tree, WStr};

const DIR: &str = concat!(env!("CARGO_MANIFEST_DIR"), "/../goldens");

#[derive(Clone, Copy)]
struct Repr {
    tag: &'static str,
    endian: Endianness,
    xcdr2: bool,
}

const REPRS: [Repr; 4] = [
    Repr { tag: "xcdr2-le", endian: Endianness::Little, xcdr2: true },
    Repr { tag: "xcdr2-be", endian: Endianness::Big, xcdr2: true },
    Repr { tag: "xcdr1-le", endian: Endianness::Little, xcdr2: false },
    Repr { tag: "xcdr1-be", endian: Endianness::Big, xcdr2: false },
];

fn writer_for(r: Repr) -> BufferWriter {
    let w = BufferWriter::new(r.endian);
    if r.xcdr2 { w.xcdr2() } else { w } // new() defaults to XCDR1 (max-align 8)
}

fn reader_for<'a>(r: Repr, bytes: &'a [u8]) -> BufferReader<'a> {
    let rd = BufferReader::new(bytes, r.endian);
    if r.xcdr2 { rd.xcdr2() } else { rd }
}

// ---- canonical samples (identical to _interop/CANONICAL.md) ----------------

fn canonical_wstr() -> WStr {
    WStr {
        label: zerodds_cdr::WString("café".to_string()),
        text: zerodds_cdr::WString("日本語\u{1F389}".to_string()),
    }
}
fn canonical_mut() -> Mut {
    Mut { a: 1_000_000, b: 2.5, c: "ok".to_string() }
}
fn canonical_bits() -> Bits {
    let mut flags = Flags::new();
    flags.set_kind(5);
    flags.set_prio(20);
    Bits { perm: Perm::READ | Perm::EXEC, flags }
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
        f32: 3.5, f64: -1234.5, b: true, o: 0xAB, ch: 0x5A,
    }
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
fn encode_one<T: CdrEncode>(name: &str, sample: &T) -> bool {
    let mut ok = true;
    for r in REPRS {
        let mut w = writer_for(r);
        if let Err(e) = sample.encode(&mut w) {
            eprintln!("FAIL encode {name}.{}: {e:?}", r.tag);
            ok = false;
            continue;
        }
        let bytes = w.into_bytes();
        let path = format!("{DIR}/{name}.{}.rust.bin", r.tag);
        if let Err(e) = std::fs::write(&path, &bytes) {
            eprintln!("FAIL write {path}: {e}");
            ok = false;
            continue;
        }
        println!(
            "ENCODE {name:<5} {tag:<8} len={len:<3} sha256={sha}",
            tag = r.tag, len = bytes.len(), sha = sha256_hex(&bytes)
        );
        println!("  hex: {}", hexline(&bytes));
    }
    ok
}

/// Encode then decode-back each repr; assert equal to the canonical sample.
fn roundtrip_one<T>(name: &str, sample: &T) -> bool
where
    T: CdrEncode + CdrDecode + PartialEq + std::fmt::Debug,
{
    let mut ok = true;
    for r in REPRS {
        let mut w = writer_for(r);
        if let Err(e) = sample.encode(&mut w) {
            eprintln!("FAIL encode {name}.{}: {e:?}", r.tag);
            ok = false;
            continue;
        }
        let bytes = w.into_bytes();
        let mut rd = reader_for(r, &bytes);
        match T::decode(&mut rd) {
            Ok(back) if &back == sample => {
                println!("RT   {name:<5} {:<8} OK ({} bytes)", r.tag, bytes.len());
            }
            Ok(back) => {
                eprintln!("FAIL roundtrip {name}.{}: decoded != canonical", r.tag);
                eprintln!("  want {sample:?}\n  got  {back:?}");
                ok = false;
            }
            Err(e) => {
                eprintln!("FAIL decode {name}.{}: {e:?}", r.tag);
                ok = false;
            }
        }
    }
    ok
}

macro_rules! for_all {
    ($f:ident) => {{
        let mut ok = true;
        ok &= $f("wstr", &canonical_wstr());
        ok &= $f("mut", &canonical_mut());
        ok &= $f("bits", &canonical_bits());
        ok &= $f("tree", &canonical_tree());
        ok &= $f("arr", &canonical_arr());
        ok &= $f("prim", &canonical_prim());
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
