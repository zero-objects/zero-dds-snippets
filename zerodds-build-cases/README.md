# Cases in the IDL-to-Rust route

Found beside [GitHub #62, #63 and #64](../zerodds-build-issues/) with a
matrix of IDL cases in the same categories, and the enum-prefix switch of
[GitHub #61](https://github.com/zero-objects/zero-dds/issues/61). Each file
under `idl/` stopped the generator or rustc before; `build.rs` generates
them with `dds_type_impls(true)`, `idl/enum_prefix.idl` also with
`strip_enum_prefix(true)`.

| IDL | What failed |
|---|---|
| `idl/names.idl` | a member named like a local of the codec (`r`, `w`, `value`, `zd_origin`, `pid`, `end`) hid it; `crate`, `Self`, `super` have no raw identifier; `gen` is reserved since the 2024 edition; a module `core` hid `core::mem`; a type `Option`, `Vec` or `Reader` hid what the codec writes without a path |
| `idl/dropped.idl` | the key-only read dropped an array inside a `sequence`, a `map` or `@external` without its length (`E0284`); a `long double` array's zero was `Float128::ZERO`, which did not resolve |
| `idl/defaults.idl` | `@default(CF)` with `const float CF` was refused; `@default(1)` on `fixed<5,2>` and on `long double` did not compile (`E0308`) |
| `idl/enum_prefix.idl` | GitHub #61, a switch that did not exist |

What the generated Rust is called now:

- a member that has a codec local's name is bound to `zd_m_<name>`; the
  field keeps its name;
- `crate`, `self`, `super`, `Self` take a trailing `_` (`crate_`, `Self_`,
  `super_`), `gen` is `r#gen`;
- a type, module or constant named like an item the code uses without a
  path takes a trailing `_` (`Option_`, `Vec_`, `Reader_`);
- with `strip_enum_prefix(true)`, `MY_ENUM_A` of `MyEnum` is `MyEnum::A`;
  `MY_ENUM_1` keeps its prefix, because `1` is no identifier.

The IDL names stay in the TypeObject, on the wire and in a content filter.

`cargo run` checks the rest:

- the key-only read (`DdsType::decode_key_members`) gives the key in XCDR2
  and XCDR1, little and big endian, and leaves the members it steps over at
  their defaults;
- the `@default` values: `2.5`, `"1.00"`, and `1` and `1.5` as `long
  double`;
- a content filter reads the enumerator's IDL name (`MY_ENUM_1`).

```sh
cargo run
```

The last line is `the IDL-to-Rust cases hold`.

## Dependencies

The fixes are in `zerodds-lang-cst`, `zerodds-lang-rules` and
`zerodds-build` 1.0.0-rc.7.6. A `long double` member needs
`zerodds-float128` beside `zerodds-dcps`, `zerodds-types` and
`zerodds-wire`. The path dependencies reach into a ZeroDDS source tree that
holds this repository, the layout of `ci/run-examples.sh` (see the
repository README); the crates.io lane of the CI builds the example against
the published crates.

## Reference

- [GitHub #61](https://github.com/zero-objects/zero-dds/issues/61), and the
  issues the matrix grew from: [#62](https://github.com/zero-objects/zero-dds/issues/62),
  [#63](https://github.com/zero-objects/zero-dds/issues/63),
  [#64](https://github.com/zero-objects/zero-dds/issues/64).
- This example in the repository: [zero-dds-snippets/zerodds-build-cases](https://github.com/zero-objects/zero-dds-snippets/tree/main/zerodds-build-cases)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
