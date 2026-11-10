# Three GitHub issues in the IDL-to-Rust route

Each file under `idl/` is the IDL of one issue as it was reported, and
`build.rs` generates it the way the reporter did:

```rust
zerodds_build::Config::new()
    .dds_type_impls(true)
    .compile(&["idl/issue62.idl", "idl/issue63.idl", "idl/issue64.idl"])
```

| Issue | IDL | What failed |
|---|---|---|
| [#62](https://github.com/zero-objects/zero-dds/issues/62) | `@key ::shapes::ShapeId id;` | the generator: `expected a declarator name` |
| [#63](https://github.com/zero-objects/zero-dds/issues/63) | `@key long id; char label[4];` in an `@appendable` struct | rustc: `E0284`, no length for a `char` array the key-only read drops |
| [#64](https://github.com/zero-objects/zero-dds/issues/64) | `@default(1) float f32_value;` | rustc: `E0308`, `f32_value: 1` |

#62: IDL 4.2 rules (4) and (218) also read the line as the annotation
`key::shapes::ShapeId` followed by a member. White space before the `::`
ends the annotation name, as in Cyclone DDS's `idlc`; `@ann::mark` without
white space stays one name. `idl/issue62.idl` also has the member with
`@optional` and `@external`.

That the crate compiles is the first half of each check. `cargo run` checks
the rest:

- every type writes and reads back in XCDR2 and XCDR1, little and big
  endian;
- the key-only read (`DdsType::decode_key_members`) gives the key and the
  KeyHash of the whole sample, and leaves the members it steps over at their
  defaults;
- the `@default(1)` members hold `1.0`.

```sh
cargo run
```

The last line is `GitHub #62, #63 and #64 hold`.

## Dependencies

The fixes are in `zerodds-idl-parse` and `zerodds-lang-rules`
1.0.0-rc.7.5. `zerodds-build` 1.0.0-rc.7.3 requires both with
`^1.0.0-rc.7`, so a `cargo update` takes them without a new
`zerodds-build`. The path dependencies reach into a ZeroDDS source tree that
holds this repository, the layout of `ci/run-examples.sh` (see the repository
README); the crates.io lane of the CI builds the example against the
published crates.

## Reference

- The issues: [#62](https://github.com/zero-objects/zero-dds/issues/62),
  [#63](https://github.com/zero-objects/zero-dds/issues/63),
  [#64](https://github.com/zero-objects/zero-dds/issues/64).
- This example in the repository: [zero-dds-snippets/zerodds-build-issues](https://github.com/zero-objects/zero-dds-snippets/tree/main/zerodds-build-issues)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
