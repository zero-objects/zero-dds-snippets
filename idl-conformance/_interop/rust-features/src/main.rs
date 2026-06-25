//! WIRE-level XCDR2 interop goldens for the ZeroDDS Rust PSM — the FEATURE
//! corpus (`features.idl`): WStr, Mut, Bits, Tree, Arr, Prim.
//!
//! For each feature this binary:
//!   * ENCODE        -> serializes the CANONICAL.md sample via the generated
//!                      `DdsType::encode` (the exact bytes a ZeroDDS DataWriter
//!                      puts on the wire, XCDR2-LE, default repr) and writes
//!                      `goldens/<feature>.rust.bin`.
//!   * DECODE <file> -> reads the bytes back via `DdsType::decode` and asserts
//!                      the reconstructed value == the canonical sample.
//!
//! The Rust golden is the cross-PSM REFERENCE: it runs through the
//! cross-vendor-validated `zerodds-cdr` core, so the other 6 language PSMs
//! conform to these bytes (CANONICAL.md "Convergence criterion").

use std::process::ExitCode;

use zerodds_dcps::DdsType;

#[path = "../generated/features.rs"]
mod generated;

use generated::feat::{
    Arr, Bits, Flags, Hue, MapEnum, MapPrim, Mut, MutLeaf, MutNest, NestedKey, OuterKey, Perm, Prim, Pt,
    Sel, Tree, WStr,
};

const DIR: &str =
    "/Users/sandrakessler/projects/zerodds/zerodds-examples/idl-conformance/_interop/goldens";

// ---------------------------------------------------------------------------
// Canonical samples (EXACT values from CANONICAL.md; identical across all PSMs)
// ---------------------------------------------------------------------------

/// feat::WStr — label="café" (c a f é/U+00E9), text="日本語🎉" (🎉 = U+1F389).
fn canonical_wstr() -> WStr {
    WStr {
        label: zerodds_cdr::WString("café".to_string()),
        text: zerodds_cdr::WString("日本語\u{1F389}".to_string()),
    }
}

/// feat::Mut — @mutable: a(@id10)=1000000, b(@id20)=2.5, c(@id30)="ok".
fn canonical_mut() -> Mut {
    Mut {
        a: 1_000_000,
        b: 2.5,
        c: "ok".to_string(),
    }
}

/// feat::MutNest — nested @mutable: tag=9, leaf={u=100,v=1.25},
/// list=[{1,0.5},{2,0.25}]. Exercises LengthCode-5 reuse on the nested
/// @mutable member (leaf) and the sequence<@mutable> member (list).
fn canonical_mutnest() -> MutNest {
    MutNest {
        tag: 9,
        leaf: MutLeaf { u: 100, v: 1.25 },
        list: vec![MutLeaf { u: 1, v: 0.5 }, MutLeaf { u: 2, v: 0.25 }],
    }
}

/// feat::OuterKey — nested-struct @key: k=NestedKey{hi=0x01020304,
/// lo=0x05060708}, payload=999. Exercises recursive @key expansion.
fn canonical_outerkey() -> OuterKey {
    OuterKey {
        k: NestedKey { hi: 0x0102_0304, lo: 0x0506_0708 },
        payload: 999,
    }
}

/// feat::Bits — perm = READ|EXEC = 0x05; flags: kind=5, prio=20 -> holder 0xA5.
fn canonical_bits() -> Bits {
    let mut flags = Flags::new();
    flags.set_kind(5);
    flags.set_prio(20);
    Bits {
        perm: Perm::READ | Perm::EXEC,
        flags,
    }
}

/// feat::Tree — root{1, [ {2,[ {4,[]} ]}, {3,[]} ]}.
fn canonical_tree() -> Tree {
    Tree {
        value: 1,
        kids: vec![
            Tree {
                value: 2,
                kids: vec![Tree { value: 4, kids: vec![] }],
            },
            Tree { value: 3, kids: vec![] },
        ],
    }
}

/// feat::Arr — grid[2][3]=[[10,11,12],[13,14,15]] row-major; shape[2]=[{1,2},{3,4}].
fn canonical_arr() -> Arr {
    Arr {
        grid: [[10, 11, 12], [13, 14, 15]],
        shape: [Pt { x: 1, y: 2 }, Pt { x: 3, y: 4 }],
    }
}

/// feat::Sel — @appendable union, discriminator 1 -> struct branch p = {5,6}.
fn canonical_sel() -> Sel {
    Sel::P(Pt { x: 5, y: 6 })
}

/// feat::MapEnum — h=H_BLUE(2); m={3:{11,12}}; sels=[Sel::N(9)].
fn canonical_mapenum() -> MapEnum {
    let mut m = ::std::collections::BTreeMap::new();
    m.insert(3, Pt { x: 11, y: 12 });
    MapEnum {
        h: Hue::H_BLUE,
        m,
        sels: vec![Sel::N(9)],
    }
}

/// feat::MapPrim — m = {7:42, 8:99}. Primitive-valued map: NO collection DHEADER.
fn canonical_mapprim() -> MapPrim {
    let mut m = ::std::collections::BTreeMap::new();
    m.insert(7, 42);
    m.insert(8, 99);
    MapPrim { m }
}

/// feat::Prim — every integer at its extreme + exact floats.
fn canonical_prim() -> Prim {
    Prim {
        i8: -128,
        u8: 255,
        i16: -32768,
        u16: 65535,
        i32: -2_147_483_648,
        u32: 4_294_967_295,
        i64: -9_223_372_036_854_775_808,
        u64: 18_446_744_073_709_551_615,
        f32: 3.5,
        f64: -1234.5,
        b: true,
        o: 0xAB,
        ch: 0x5A, // 'Z'
    }
}

// ---------------------------------------------------------------------------
// Encode / decode drivers
// ---------------------------------------------------------------------------

fn hexdump(bytes: &[u8]) {
    print!("hex:");
    for (i, b) in bytes.iter().enumerate() {
        if i % 16 == 0 {
            print!("\n  ");
        }
        print!("{b:02x} ");
    }
    println!();
}

/// Encode one feature, write its golden, print bytes.
fn encode_one<T: DdsType>(name: &str, sample: &T) -> bool {
    let mut bytes = Vec::new();
    match sample.encode(&mut bytes) {
        Ok(()) => {}
        Err(e) => {
            eprintln!("FAIL: encode {name}: {e:?}");
            return false;
        }
    }
    let path = format!("{DIR}/{name}.rust.bin");
    if let Err(e) = std::fs::write(&path, &bytes) {
        eprintln!("FAIL: write {path}: {e}");
        return false;
    }
    println!("ENCODE {name}: {} bytes -> {path}", bytes.len());
    hexdump(&bytes);
    true
}

/// Decode one feature's golden back and assert equality with the canonical.
fn decode_one<T: DdsType + PartialEq + std::fmt::Debug>(name: &str, expected: &T) -> bool {
    let path = format!("{DIR}/{name}.rust.bin");
    let bytes = match std::fs::read(&path) {
        Ok(b) => b,
        Err(e) => {
            eprintln!("FAIL: read {path}: {e}");
            return false;
        }
    };
    let got = match T::decode(&bytes) {
        Ok(t) => t,
        Err(e) => {
            eprintln!("FAIL: decode {name}: {e:?}");
            return false;
        }
    };
    if &got == expected {
        println!("OK {name}: decoded == canonical ({} bytes)", bytes.len());
        true
    } else {
        eprintln!("FAIL {name}: decoded != canonical");
        eprintln!("  expected {expected:?}");
        eprintln!("  got      {got:?}");
        false
    }
}

fn run_encode() -> bool {
    let mut ok = true;
    ok &= encode_one("wstr", &canonical_wstr());
    ok &= encode_one("mut", &canonical_mut());
    ok &= encode_one("bits", &canonical_bits());
    ok &= encode_one("tree", &canonical_tree());
    ok &= encode_one("arr", &canonical_arr());
    ok &= encode_one("prim", &canonical_prim());
    ok &= encode_one("mutnest", &canonical_mutnest());
    ok &= encode_one("outerkey", &canonical_outerkey());
    ok &= encode_one("mapenum", &canonical_mapenum());
    ok &= encode_one("mapprim", &canonical_mapprim());
    ok
}

fn run_decode() -> bool {
    let mut ok = true;
    ok &= decode_one("wstr", &canonical_wstr());
    ok &= decode_one("mut", &canonical_mut());
    ok &= decode_one("bits", &canonical_bits());
    ok &= decode_one("tree", &canonical_tree());
    ok &= decode_one("arr", &canonical_arr());
    ok &= decode_one("prim", &canonical_prim());
    ok &= decode_one("mutnest", &canonical_mutnest());
    ok &= decode_one("outerkey", &canonical_outerkey());
    ok &= decode_one("mapenum", &canonical_mapenum());
    ok &= decode_one("mapprim", &canonical_mapprim());
    ok
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Each feature golden encodes its CANONICAL.md sample and decodes back
    /// to the identical value (self-consistency of the Rust reference PSM).
    #[test]
    fn feature_goldens_roundtrip() {
        macro_rules! rt {
            ($s:expr) => {{
                let sample = $s;
                let mut bytes = Vec::new();
                sample.encode(&mut bytes).expect("encode");
                let back = ::zerodds_dcps::DdsType::decode(&bytes).expect("decode");
                assert_eq!(sample, back, "feature golden must roundtrip");
            }};
        }
        rt!(canonical_wstr());
        rt!(canonical_mut());
        rt!(canonical_bits());
        rt!(canonical_tree());
        rt!(canonical_arr());
        rt!(canonical_prim());
        rt!(canonical_mutnest());
        rt!(canonical_outerkey());
        rt!(canonical_mapenum());
    }
}

fn main() -> ExitCode {
    let args: Vec<String> = std::env::args().collect();
    let ok = match args.get(1).map(String::as_str) {
        Some("ENCODE") => run_encode(),
        Some("DECODE") => run_decode(),
        Some("ROUNDTRIP") | None => run_encode() && run_decode(),
        _ => {
            eprintln!("usage: features_interop ENCODE | DECODE | ROUNDTRIP");
            return ExitCode::FAILURE;
        }
    };
    if ok {
        ExitCode::SUCCESS
    } else {
        ExitCode::FAILURE
    }
}
