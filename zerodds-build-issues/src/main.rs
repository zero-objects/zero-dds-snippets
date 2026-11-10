// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 ZeroDDS Contributors
//! GitHub #62, #63 and #64, each with the reporter's IDL (`idl/`), generated
//! by `build.rs` with `dds_type_impls(true)` as in the issues.
//!
//! That the crate compiles is the first half of each check: #62 stopped
//! the generator, #63 and #64 stopped rustc. `cargo run` checks the rest:
//! every type writes and reads back in XCDR2 and XCDR1, little and big
//! endian; the key-only read gives the key and the KeyHash of the whole
//! sample; and the `@default(1)` members hold 1.0.

use zerodds_dcps::DdsType;

#[allow(
    dead_code,
    missing_docs,
    non_camel_case_types,
    non_snake_case,
    unused_imports
)]
mod issue62 {
    include!(concat!(env!("OUT_DIR"), "/issue62.rs"));
}

#[allow(
    dead_code,
    missing_docs,
    non_camel_case_types,
    non_snake_case,
    unused_imports
)]
mod issue63 {
    include!(concat!(env!("OUT_DIR"), "/issue63.rs"));
}

#[allow(
    dead_code,
    missing_docs,
    non_camel_case_types,
    non_snake_case,
    unused_imports
)]
mod issue64 {
    include!(concat!(env!("OUT_DIR"), "/issue64.rs"));
}

/// The four encodings: XCDR2 and XCDR1, each little and big endian.
const MODES: [(&str, bool, bool); 4] = [
    ("XCDR2 LE", false, false),
    ("XCDR2 BE", false, true),
    ("XCDR1 LE", true, false),
    ("XCDR1 BE", true, true),
];

fn write<T: DdsType>(value: &T, xcdr1: bool, big_endian: bool) -> Vec<u8> {
    let mut out = Vec::new();
    match (xcdr1, big_endian) {
        (false, false) => T::encode(value, &mut out),
        (false, true) => T::encode_be(value, &mut out),
        (true, false) => T::encode_xcdr1(value, &mut out),
        (true, true) => T::encode_xcdr1_be(value, &mut out),
    }
    .expect("the sample encodes");
    out
}

fn read<T: DdsType>(bytes: &[u8], xcdr1: bool, big_endian: bool) -> T {
    match (xcdr1, big_endian) {
        (false, false) => T::decode(bytes),
        (false, true) => T::decode_be(bytes),
        (true, false) => T::decode_xcdr1(bytes),
        (true, true) => T::decode_xcdr1_be(bytes),
    }
    .expect("the sample decodes")
}

fn keys<T: DdsType>(bytes: &[u8], xcdr1: bool, big_endian: bool) -> T {
    T::decode_key_members(bytes, xcdr1, big_endian)
        .expect("a generated type has a key-only read")
        .expect("the key-only read succeeds")
}

/// #62: `@key ::shapes::ShapeId id;` generates, and so do the same member
/// with `@optional` and `@external`.
fn issue_62() {
    let shape = issue62::shapes::Shape {
        id: issue62::shapes::ShapeId { value: 7 },
    };
    for (mode, xcdr1, big_endian) in MODES {
        let bytes = write(&shape, xcdr1, big_endian);
        let back: issue62::shapes::Shape = read(&bytes, xcdr1, big_endian);
        assert_eq!(back.id.value, 7, "{mode}: the sample reads back");
        let key: issue62::shapes::Shape = keys(&bytes, xcdr1, big_endian);
        assert_eq!(key.id.value, 7, "{mode}: the key-only read");
        assert_eq!(
            DdsType::key_hash(&key),
            DdsType::key_hash(&shape),
            "{mode}: the KeyHash"
        );
    }
    let mut optional = issue62::shapes::ShapeWithOptional::default();
    optional.index = 3;
    let mut external = issue62::shapes::ShapeWithExternal::default();
    external.index = 4;
    for (mode, xcdr1, big_endian) in MODES {
        let back: issue62::shapes::ShapeWithOptional =
            read(&write(&optional, xcdr1, big_endian), xcdr1, big_endian);
        assert_eq!(back.index, 3, "{mode}: @optional reads back");
        let back: issue62::shapes::ShapeWithExternal =
            read(&write(&external, xcdr1, big_endian), xcdr1, big_endian);
        assert_eq!(back.index, 4, "{mode}: @external reads back");
    }
    println!(
        "#62 `@key ::shapes::ShapeId id;` generates, with @optional and @external too; \
         id.value 7 read back and read as the key in 4 encodings"
    );
}

/// #63: a keyed `@appendable` struct with a non-key `char label[4]`.
fn issue_63() {
    let sample = issue63::Sample {
        id: 42,
        label: ['Z', 'D', 'D', 'S'],
    };
    for (mode, xcdr1, big_endian) in MODES {
        let bytes = write(&sample, xcdr1, big_endian);
        let back: issue63::Sample = read(&bytes, xcdr1, big_endian);
        assert_eq!(back.id, 42, "{mode}: id reads back");
        assert_eq!(back.label, ['Z', 'D', 'D', 'S'], "{mode}: label reads back");
        let key: issue63::Sample = keys(&bytes, xcdr1, big_endian);
        assert_eq!(key.id, 42, "{mode}: the key-only read steps over label");
        assert_eq!(
            DdsType::key_hash(&key),
            DdsType::key_hash(&sample),
            "{mode}: the KeyHash"
        );
    }
    println!(
        "#63 `char label[4]` beside `@key long id` compiles; label read back, \
         the key-only read gives id 42 in 4 encodings"
    );
}

/// #64: `@default(1)` on a `float` and a `double` member.
fn issue_64() {
    let sample = issue64::Sample::default();
    assert_eq!(sample.f32_value, 1.0_f32, "the float default");
    assert_eq!(sample.f64_value, 1.0_f64, "the double default");
    let mut keyed = issue64::Sample::default();
    keyed.id = 5;
    keyed.f32_value = 2.5;
    for (mode, xcdr1, big_endian) in MODES {
        let key: issue64::Sample = keys(&write(&keyed, xcdr1, big_endian), xcdr1, big_endian);
        assert_eq!(key.id, 5, "{mode}: the key");
        assert_eq!(
            key.f32_value, 1.0_f32,
            "{mode}: a member the key-only read steps over is at its default"
        );
    }
    println!(
        "#64 `@default(1)` on float and double compiles; f32_value {} and f64_value {}",
        sample.f32_value, sample.f64_value
    );
}

fn main() {
    issue_62();
    issue_63();
    issue_64();
    println!("GitHub #62, #63 and #64 hold");
}
