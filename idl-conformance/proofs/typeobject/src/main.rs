//! TypeObject / TypeIdentifier byte-identity PROOF harness (DDS-XTypes 1.3
//! §7.3.4).
//!
//! For each canonical type in `idl/typeobject.idl` this harness builds the
//! **MinimalTypeObject** and **CompleteTypeObject** exactly as ZeroDDS would
//! register them, then prints, for both:
//!
//!   * the serialized TypeObject bytes (len + sha256 + hexdump),
//!   * the 14-byte `EquivalenceHash` = first 14 bytes of MD5 over the
//!     serialized TypeObject (XTypes 1.3 §7.3.1.2.1 / §7.3.4.5), and
//!   * the resulting `TypeIdentifier` (kind byte `EK_MINIMAL=0xF1` /
//!     `EK_COMPLETE=0xF2` + the 14-byte hash).
//!
//! It writes the bytes to `goldens/<case>.{minimal,complete}.zerodds.bin` and
//! the 14-byte hashes to `goldens/<case>.{minimal,complete}.hash`.
//!
//! With `--check` it byte-compares each ZeroDDS TypeObject + hash against the
//! recorded vendor goldens under `vendor/<vendor>/` and prints a MATCH/DIFF
//! table. See README.md for the spec basis, the per-type interop property, the
//! vendor versions, and reproduction steps (incl. how each vendor's bytes were
//! extracted).
//!
//! NOTE on serialization: XTypes 1.3 §7.3.4.5 mandates the TypeObject be
//! serialized with **XCDR2, Little-Endian**, with members ordered by
//! member_index (struct/union) / value (enum). The bytes printed here are
//! whatever `zerodds_types::hash::compute_*_hash` feeds to MD5 — i.e. exactly
//! the bytes ZeroDDS hashes. If those diverge from a vendor's XCDR2 framing,
//! that divergence is the finding (see README "Findings").

use std::path::PathBuf;
use std::process::ExitCode;

use zerodds_types::builder::{Extensibility, TypeObjectBuilder};
use zerodds_types::hash::{compute_complete_hash, compute_minimal_hash};
use zerodds_types::type_object::{CompleteTypeObject, MinimalTypeObject};
use zerodds_types::{EquivalenceHash, PrimitiveKind, TypeIdentifier};

const EK_MINIMAL: u8 = 0xF1;
const EK_COMPLETE: u8 = 0xF2;

fn dir(sub: &str) -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR")).join(sub)
}

// ---------------------------------------------------------------------------
// Serialized-TypeObject bytes that go INTO the MD5 (i.e. what gets hashed).
//
// `compute_minimal_hash` / `compute_complete_hash` write the EquivalenceKind
// discriminator (0xF1 / 0xF2) and then the TypeObject body, then MD5 it. We
// reproduce that exact framing here so the printed bytes ARE the hash input.
// ---------------------------------------------------------------------------

fn minimal_to_bytes(t: &MinimalTypeObject) -> Vec<u8> {
    use zerodds_types::type_object::TypeObject;
    TypeObject::Minimal(t.clone())
        .to_bytes_le()
        .expect("encode minimal")
}

fn complete_to_bytes(t: &CompleteTypeObject) -> Vec<u8> {
    use zerodds_types::type_object::TypeObject;
    TypeObject::Complete(t.clone())
        .to_bytes_le()
        .expect("encode complete")
}

// ---------------------------------------------------------------------------
// Canonical type builders — identical shapes to idl/typeobject.idl.
// ---------------------------------------------------------------------------

fn plain() -> (MinimalTypeObject, CompleteTypeObject) {
    let b = TypeObjectBuilder::struct_type("::to::Plain")
        .extensibility(Extensibility::Final)
        .member("a", TypeIdentifier::Primitive(PrimitiveKind::Int32), |m| m)
        .member("b", TypeIdentifier::Primitive(PrimitiveKind::Int16), |m| m);
    (
        MinimalTypeObject::Struct(b.build_minimal()),
        CompleteTypeObject::Struct(b.build_complete()),
    )
}

fn appendable() -> (MinimalTypeObject, CompleteTypeObject) {
    let b = TypeObjectBuilder::struct_type("::to::Appendable")
        .extensibility(Extensibility::Appendable)
        .member("a", TypeIdentifier::Primitive(PrimitiveKind::Int32), |m| m)
        .member("b", TypeIdentifier::Primitive(PrimitiveKind::Int16), |m| m);
    (
        MinimalTypeObject::Struct(b.build_minimal()),
        CompleteTypeObject::Struct(b.build_complete()),
    )
}

fn mutable() -> (MinimalTypeObject, CompleteTypeObject) {
    let b = TypeObjectBuilder::struct_type("::to::Mutable")
        .extensibility(Extensibility::Mutable)
        .member("a", TypeIdentifier::Primitive(PrimitiveKind::Int32), |m| {
            m.id(10)
        })
        .member("b", TypeIdentifier::Primitive(PrimitiveKind::Int16), |m| {
            m.id(20)
        });
    (
        MinimalTypeObject::Struct(b.build_minimal()),
        CompleteTypeObject::Struct(b.build_complete()),
    )
}

fn color_enum() -> (MinimalTypeObject, CompleteTypeObject) {
    let b = TypeObjectBuilder::enum_type("::to::Color")
        .literal("RED", 0)
        .literal("GREEN", 1)
        .literal("BLUE", 2);
    (
        MinimalTypeObject::Enumerated(b.build_minimal()),
        CompleteTypeObject::Enumerated(b.build_complete()),
    )
}

fn long_seq4_alias() -> (MinimalTypeObject, CompleteTypeObject) {
    // typedef sequence<long,4> LongSeq4;
    let elem = TypeIdentifier::Primitive(PrimitiveKind::Int32);
    let seq_ti = TypeIdentifier::PlainSequenceSmall {
        header: zerodds_types::PlainCollectionHeader {
            equiv_kind: 0x00, // EK_BOTH for a fully-descriptive element
            element_flags: zerodds_types::type_identifier::CollectionElementFlag(0),
        },
        bound: 4,
        element: Box::new(elem),
    };
    let b = TypeObjectBuilder::alias("::to::LongSeq4", seq_ti);
    (
        MinimalTypeObject::Alias(b.build_minimal()),
        CompleteTypeObject::Alias(b.build_complete()),
    )
}

fn inner() -> (MinimalTypeObject, CompleteTypeObject) {
    let b = TypeObjectBuilder::struct_type("::to::Inner")
        .extensibility(Extensibility::Final)
        .member("x", TypeIdentifier::Primitive(PrimitiveKind::Int32), |m| m)
        .member("y", TypeIdentifier::Primitive(PrimitiveKind::Int32), |m| m);
    (
        MinimalTypeObject::Struct(b.build_minimal()),
        CompleteTypeObject::Struct(b.build_complete()),
    )
}

/// `Nested` references `Inner` (by its EK_MINIMAL/EK_COMPLETE hash),
/// `Color` (by hash), a bounded sequence<long,8> and a string<64>.
fn nested(
    inner_min_hash: EquivalenceHash,
    inner_cmp_hash: EquivalenceHash,
    color_min_hash: EquivalenceHash,
    color_cmp_hash: EquivalenceHash,
) -> (MinimalTypeObject, CompleteTypeObject) {
    let seq_long8 = TypeIdentifier::PlainSequenceSmall {
        header: zerodds_types::PlainCollectionHeader {
            equiv_kind: 0x00,
            element_flags: zerodds_types::type_identifier::CollectionElementFlag(0),
        },
        bound: 8,
        element: Box::new(TypeIdentifier::Primitive(PrimitiveKind::Int32)),
    };
    let name_str = TypeIdentifier::String8Small { bound: 64 };

    let min = TypeObjectBuilder::struct_type("::to::Nested")
        .extensibility(Extensibility::Appendable)
        .member(
            "child",
            TypeIdentifier::EquivalenceHashMinimal(inner_min_hash),
            |m| m,
        )
        .member("seq", seq_long8.clone(), |m| m)
        .member("name", name_str.clone(), |m| m)
        .member(
            "color",
            TypeIdentifier::EquivalenceHashMinimal(color_min_hash),
            |m| m,
        )
        .build_minimal();

    let cmp = TypeObjectBuilder::struct_type("::to::Nested")
        .extensibility(Extensibility::Appendable)
        .member(
            "child",
            TypeIdentifier::EquivalenceHashComplete(inner_cmp_hash),
            |m| m,
        )
        .member("seq", seq_long8, |m| m)
        .member("name", name_str, |m| m)
        .member(
            "color",
            TypeIdentifier::EquivalenceHashComplete(color_cmp_hash),
            |m| m,
        )
        .build_complete();

    (
        MinimalTypeObject::Struct(min),
        CompleteTypeObject::Struct(cmp),
    )
}

fn choice_union() -> (MinimalTypeObject, CompleteTypeObject) {
    let b = TypeObjectBuilder::union_type(
        "::to::Choice",
        TypeIdentifier::Primitive(PrimitiveKind::Int32),
    )
    .extensibility(Extensibility::Final)
    .case(
        "as_long",
        TypeIdentifier::Primitive(PrimitiveKind::Int32),
        vec![1],
    )
    .case(
        "as_double",
        TypeIdentifier::Primitive(PrimitiveKind::Float64),
        vec![2],
    )
    .default_case("as_text", TypeIdentifier::String8Small { bound: 32 });
    (
        MinimalTypeObject::Union(b.build_minimal()),
        CompleteTypeObject::Union(b.build_complete()),
    )
}

// ---------------------------------------------------------------------------
// sha256 (self-contained, FIPS 180-4) — for stable byte fingerprints.
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

fn hex14(h: &EquivalenceHash) -> String {
    h.0.iter().map(|b| format!("{b:02x}")).collect()
}

fn write_bytes(name: &str, bytes: &[u8]) {
    std::fs::create_dir_all(dir("goldens")).ok();
    std::fs::write(dir("goldens").join(name), bytes).expect("write golden");
}

struct CaseOut {
    name: &'static str,
    min_bytes: Vec<u8>,
    min_hash: EquivalenceHash,
    cmp_bytes: Vec<u8>,
    cmp_hash: EquivalenceHash,
}

fn build_case(
    name: &'static str,
    min: &MinimalTypeObject,
    cmp: &CompleteTypeObject,
) -> CaseOut {
    CaseOut {
        name,
        min_bytes: minimal_to_bytes(min),
        min_hash: compute_minimal_hash(min).expect("min hash"),
        cmp_bytes: complete_to_bytes(cmp),
        cmp_hash: compute_complete_hash(cmp).expect("cmp hash"),
    }
}

fn main() -> ExitCode {
    let check = std::env::args().any(|a| a == "--check");

    // Inner + Color first — Nested references their hashes.
    let (inner_min, inner_cmp) = inner();
    let (color_min, color_cmp) = color_enum();
    let inner_min_hash = compute_minimal_hash(&inner_min).unwrap();
    let inner_cmp_hash = compute_complete_hash(&inner_cmp).unwrap();
    let color_min_hash = compute_minimal_hash(&color_min).unwrap();
    let color_cmp_hash = compute_complete_hash(&color_cmp).unwrap();
    let (nested_min, nested_cmp) = nested(
        inner_min_hash,
        inner_cmp_hash,
        color_min_hash,
        color_cmp_hash,
    );

    let (plain_min, plain_cmp) = plain();
    let (app_min, app_cmp) = appendable();
    let (mut_min, mut_cmp) = mutable();
    let (alias_min, alias_cmp) = long_seq4_alias();
    let (union_min, union_cmp) = choice_union();

    let cases = vec![
        build_case("plain", &plain_min, &plain_cmp),
        build_case("appendable", &app_min, &app_cmp),
        build_case("mutable", &mut_min, &mut_cmp),
        build_case("color_enum", &color_min, &color_cmp),
        build_case("long_seq4_alias", &alias_min, &alias_cmp),
        build_case("inner", &inner_min, &inner_cmp),
        build_case("nested", &nested_min, &nested_cmp),
        build_case("choice_union", &union_min, &union_cmp),
    ];

    println!("=== ZeroDDS TypeObject serialization + TypeIdentifier hash ===");
    println!("(EK_MINIMAL=0x{EK_MINIMAL:02x}  EK_COMPLETE=0x{EK_COMPLETE:02x})\n");

    for c in &cases {
        write_bytes(&format!("{}.minimal.zerodds.bin", c.name), &c.min_bytes);
        write_bytes(&format!("{}.complete.zerodds.bin", c.name), &c.cmp_bytes);
        write_bytes(
            &format!("{}.minimal.hash", c.name),
            hex14(&c.min_hash).as_bytes(),
        );
        write_bytes(
            &format!("{}.complete.hash", c.name),
            hex14(&c.cmp_hash).as_bytes(),
        );

        println!("[{}]", c.name);
        println!(
            "  MINIMAL : {:>4} B  sha256={}",
            c.min_bytes.len(),
            sha256(&c.min_bytes)
        );
        println!("    bytes: {}", hexdump(&c.min_bytes));
        println!(
            "    TypeIdentifier: kind=0x{:02x} hash={}",
            EK_MINIMAL,
            hex14(&c.min_hash)
        );
        println!(
            "  COMPLETE: {:>4} B  sha256={}",
            c.cmp_bytes.len(),
            sha256(&c.cmp_bytes)
        );
        println!("    bytes: {}", hexdump(&c.cmp_bytes));
        println!(
            "    TypeIdentifier: kind=0x{:02x} hash={}",
            EK_COMPLETE,
            hex14(&c.cmp_hash)
        );
        println!();
    }

    if !check {
        println!("(run with --check to byte-compare against recorded vendor goldens)");
        return ExitCode::SUCCESS;
    }

    let vendors = ["cyclonedds", "rti", "fastdds"];
    println!("=== byte-diff vs vendor goldens (TypeObject bytes + 14-byte hash) ===\n");
    println!(
        "{:<18} {:<11} {:<9} {:<12} {}",
        "case", "vendor", "kind", "result", "note"
    );

    for c in &cases {
        for (kind, zbytes, zhash) in [
            ("MINIMAL", &c.min_bytes, &c.min_hash),
            ("COMPLETE", &c.cmp_bytes, &c.cmp_hash),
        ] {
            for v in vendors {
                let bin = dir("vendor")
                    .join(v)
                    .join(format!("{}.{}.bin", c.name, kind.to_lowercase()));
                let hashf = dir("vendor")
                    .join(v)
                    .join(format!("{}.{}.hash", c.name, kind.to_lowercase()));
                let vbytes = std::fs::read(&bin).ok();
                let vhash = std::fs::read_to_string(&hashf)
                    .ok()
                    .map(|s| s.trim().to_lowercase());

                let (res, note) = match (vbytes, vhash) {
                    (Some(vb), vh) => {
                        let byte_match = vb == *zbytes;
                        let hash_match = vh
                            .as_deref()
                            .map(|h| h == hex14(zhash))
                            .unwrap_or(false);
                        let r = if byte_match && hash_match {
                            "MATCH"
                        } else if byte_match {
                            "BYTE-OK/HASH?"
                        } else {
                            "DIFF"
                        };
                        (
                            r,
                            format!("zerodds {}B vs {} {}B", zbytes.len(), v, vb.len()),
                        )
                    }
                    (None, Some(vh)) => {
                        // Only a hash recorded (e.g. extracted on the wire).
                        let r = if vh == hex14(zhash) { "HASH-MATCH" } else { "HASH-DIFF" };
                        (r, format!("hash-only: vendor={vh}"))
                    }
                    (None, None) => ("n/a", "no vendor golden".to_string()),
                };
                println!(
                    "{:<18} {:<11} {:<9} {:<12} {}",
                    c.name, v, kind, res, note
                );
            }
        }
        println!();
    }

    ExitCode::SUCCESS
}
