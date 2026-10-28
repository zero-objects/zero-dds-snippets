// SPDX-License-Identifier: Apache-2.0
//! KeyHash conformance proof harness.
//!
//! Computes the 16-byte DDSI-RTPS PID_KEY_HASH (0x0070) that ZeroDDS would
//! place in the inline QoS of a DATA submessage, for one fixed instance of
//! each canonical keyed type, and prints it as hex. Compare the output with
//! the committed vendor captures in `vendor/` (see README.md).
//!
//! Rule (XTypes 1.3 §7.6.8 / DDSI-RTPS 2.5 §9.6.3.8):
//!   * Serialize the @key members in big-endian PLAIN_CDR2, member-id order.
//!   * If the *statically-known max* keyholder size <= 16 bytes: use those
//!     bytes, zero-padded to 16.
//!   * Otherwise: KeyHash = MD5(keyholder bytes).
//!
//! ## Case (d) does not compile against rc.7
//!
//! `zerodds-build` is the successor to the `zerodds-idl` + `zerodds-idl-rust`
//! pair this file was written against. It generates cases (a), (b) and (c),
//! and miscompiles (d): for a `@key` member of aggregate type the inherent
//! `key_holder_be()` and `key_hash()` come out as
//!
//! ```ignore
//! let mut holder = Writer::with_max_align(zerodds_wire::Endianness::Big, 4);
//! self.k.encode(holder)?;          // error[E0308]: expected `&mut Writer`
//! ```
//!
//! Every other sink in the emitter is already a `&mut Writer` parameter, so
//! the `MemberWrite::Nested` arm of `write_call` (crates/lang-rules/src/
//! encode.rs) hands the sink over by name; here the sink is an owned `Writer`
//! and the name moves it. `key_from_holder_be()` for the same shape is emitted
//! as an unconditional `WireError::refused(..., "a key holder with a member
//! this emitter cannot read")`, so `DdsType::decode_key_holder_be` would refuse
//! case (d) at run time as well.

// `zerodds-build` writes the module into OUT_DIR; the lint allows sit on the
// wrapping module because `include!` splices the tokens into this file.
#[allow(non_snake_case, non_camel_case_types, unused_imports, dead_code, clippy::all)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/keyhash_cases.rs"));
}

use generated::keyhash::{BigKey, BoundedKey, MixedKeys, NestedKey, OuterKey, SingleKey};
use zerodds_dcps::key_hash::PlainCdr2BeKeyHolder;
use zerodds_dcps::DdsType;

fn hex16(b: &[u8; 16]) -> String {
    b.iter().map(|x| format!("{x:02x}")).collect()
}

/// The vendor KeyHashes vendor/RECORDED-KEYHASHES.txt records for the five
/// instances: (a)-(d) identical in Cyclone, RTI (wire) and Fast DDS, (e)
/// Cyclone's own CDR engine.
const RECORDED: [(&str, &str); 5] = [
    ("a", "11223344000000000000000000000000"),
    ("b", "492ce15eccc6c49993afe63223435b91"),
    ("c", "aa001234556677885a00000000000000"),
    ("d", "01020304050607080000000000000000"),
    ("e", "00000003414200000000000000000000"),
];

fn main() -> std::process::ExitCode {
    let check = std::env::args().any(|a| a == "--check");
    println!("# ZeroDDS KeyHash proof  (PID_KEY_HASH 0x0070, 16 bytes)\n");
    println!("# type / instance -> 16-byte KeyHash (hex)\n");

    // ---- Case (a): single @key long, zero-pad path ----
    // id = 0x11223344. Keyholder = BE(i32) = 11 22 33 44, max=4 -> zero-pad.
    let a = SingleKey {
        id: 0x1122_3344,
        payload: 999,
    };
    let ka = a.compute_key_hash().expect("keyed");
    println!("(a) SingleKey  {{ id = 0x11223344 }}");
    println!("    max_size = {:?}  (<=16 -> zero-pad)", SingleKey::KEY_HOLDER_MAX_SIZE);
    println!("    KeyHash  = {}\n", hex16(&ka));

    // ---- Case (b): @key string (unbounded) + @key long, MD5 path ----
    // name = "sensor-array-north-wing", region = 7.
    // Keyholder = CDR-string(len BE + utf8 + NUL) then BE(i32). Unbounded
    // string -> no static max -> MD5.
    let b = BigKey {
        name: "sensor-array-north-wing".to_string(),
        region: 7,
        payload: 999,
    };
    let kb = b.compute_key_hash().expect("keyed");
    println!("(b) BigKey  {{ name = \"sensor-array-north-wing\", region = 7 }}");
    println!("    max_size = {:?}  (None -> MD5)", BigKey::KEY_HOLDER_MAX_SIZE);
    // Show the keyholder bytes we MD5'd, for transparency.
    let mut hb = PlainCdr2BeKeyHolder::new();
    b.encode_key_holder_be(&mut hb);
    println!("    keyholder bytes ({} B) = {}", hb.as_bytes().len(),
        hb.as_bytes().iter().map(|x| format!("{x:02x}")).collect::<String>());
    println!("    KeyHash  = MD5(above) = {}\n", hex16(&kb));

    // ---- Case (c): multiple mixed @key fields, zero-pad path ----
    // a=0xAA, b=0x1234, c=0x55667788, d='Z'(0x5A).
    let c = MixedKeys {
        a: 0xAA,
        b: 0x1234,
        c: 0x5566_7788,
        d: 'Z',
        payload: 999,
    };
    let kc = c.compute_key_hash().expect("keyed");
    println!("(c) MixedKeys  {{ a=0xAA, b=0x1234, c=0x55667788, d='Z' }}");
    println!("    max_size = {:?}  (<=16 -> zero-pad)", MixedKeys::KEY_HOLDER_MAX_SIZE);
    let mut hc = PlainCdr2BeKeyHolder::new();
    c.encode_key_holder_be(&mut hc);
    println!("    keyholder bytes ({} B) = {}", hc.as_bytes().len(),
        hc.as_bytes().iter().map(|x| format!("{x:02x}")).collect::<String>());
    println!("    KeyHash  = {}\n", hex16(&kc));

    // ---- Case (d): nested @key struct, GENERATED keyholder (FINDING F2) ----
    // The ZeroDDS Rust backend now expands `@key NestedKey k` into the nested
    // struct's own @key members (hi, lo) in member-id order — each a BE i32 —
    // so `OuterKey::compute_key_hash()` / `encode_key_holder_be()` are fully
    // generated. NestedKey{ hi=0x01020304, lo=0x05060708 } -> 8 bytes,
    // max <= 16 -> zero-pad. (Previously hand-built because codegen rejected
    // the nested-struct @key member.)
    let d = OuterKey {
        k: NestedKey { hi: 0x0102_0304, lo: 0x0506_0708 },
        payload: 999,
    };
    let kd = d.compute_key_hash().expect("keyed");
    println!("(d) OuterKey {{ k = NestedKey {{ hi=0x01020304, lo=0x05060708 }} }}  (generated keyholder)");
    println!("    max_size = {:?}  (<=16 -> zero-pad)", OuterKey::KEY_HOLDER_MAX_SIZE);
    let mut hd = PlainCdr2BeKeyHolder::new();
    d.encode_key_holder_be(&mut hd);
    println!("    keyholder bytes ({} B) = {}", hd.as_bytes().len(),
        hd.as_bytes().iter().map(|x| format!("{x:02x}")).collect::<String>());
    println!("    KeyHash  = {}\n", hex16(&kd));

    // ---- Case (e): @key string<8>, zero-pad path (FINDING F1) ----
    let e = BoundedKey {
        code: "AB".to_string(),
        payload: 999,
    };
    let ke = e.compute_key_hash().expect("keyed");
    println!("(e) BoundedKey {{ code = \"AB\" }}  (@key string<8>)");
    println!("    max_size = {:?}  (<=16 -> zero-pad)", BoundedKey::KEY_HOLDER_MAX_SIZE);
    let mut he = PlainCdr2BeKeyHolder::new();
    e.encode_key_holder_be(&mut he);
    println!("    keyholder bytes ({} B) = {}", he.as_bytes().len(),
        he.as_bytes().iter().map(|x| format!("{x:02x}")).collect::<String>());
    println!("    KeyHash  = {}\n", hex16(&ke));

    if !check {
        println!("(run with --check to compare against vendor/RECORDED-KEYHASHES.txt)");
        return std::process::ExitCode::SUCCESS;
    }
    let ours = [ka, kb, kc, kd, ke];
    let mut mismatches = 0;
    println!("case | ZeroDDS                          | recorded                         | result");
    for ((case, recorded), key) in RECORDED.iter().zip(ours.iter()) {
        let zerodds = hex16(key);
        let result = if zerodds == *recorded { "MATCH" } else { mismatches += 1; "DIFF" };
        println!(" ({case}) | {zerodds} | {recorded} | {result}");
    }
    if mismatches != 0 {
        println!("{mismatches} of {} KeyHashes differ from the recorded vendor values", RECORDED.len());
        return std::process::ExitCode::FAILURE;
    }
    println!("ALL {} KEYHASHES MATCH THE RECORDED VENDOR VALUES", RECORDED.len());
    std::process::ExitCode::SUCCESS
}
