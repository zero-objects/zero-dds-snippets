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

#[path = "../generated/keyhash_cases.rs"]
mod generated;

use generated::keyhash::{BigKey, MixedKeys, NestedKey, OuterKey, SingleKey};
use zerodds_cdr::PlainCdr2BeKeyHolder;
use zerodds_dcps::DdsType;

fn hex16(b: &[u8; 16]) -> String {
    b.iter().map(|x| format!("{x:02x}")).collect()
}

fn main() {
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
        d: b'Z',
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
}
