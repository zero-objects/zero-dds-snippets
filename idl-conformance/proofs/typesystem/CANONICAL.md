# Canonical samples — exact values (identical for ZeroDDS and every vendor)

Each case below is encoded to XCDR2 little-endian (the 4-byte CDR encapsulation
header is stripped from every golden). The values are byte-for-byte the same
inputs on the ZeroDDS side (`src/main.rs`) and on every vendor serialiser
(`vendor/_serializers/*`). See `README.md` for what each case proves.

## `enum_holder`  (`ts::EnumHolder`, `@final`)
- `big`    (`Color`, 32-bit)         = `BLUE`  (= 2)
- `small`  (`Tiny`, `@bit_bound(8)`)  = `T_C`   (= 2)
- `medium` (`Mid`, `@bit_bound(16)`)  = `MID_D` (= 3)

## `union_long_default`  (`ts::LongUnion switch(long)`, `@final`)
- default arm selected → `note` (`string`) = `"hi"`
  (discriminator is any value that is not an explicit case label; ZeroDDS = 0)

## `union_long_struct`  (`ts::LongUnion`)
- case 2 → `where` (`Point`) = `{ x = 7, y = 8 }`

## `union_enum_default`  (`ts::EnumUnion switch(Color)`, `@final`)
- default arm selected (discriminator `BLUE`) → `fallback` (`long`) = `42`

## `opt_holder`  (`ts::OptHolder`, `@appendable`)
- `id`        (`long`)            = `1`
- `maybe_num` (`@optional long`)  = `77`     (PRESENT)
- `maybe_str` (`@optional string`)= absent   (ABSENT)
- `maybe_obj` (`@optional Inner`) = `{ a = 5, b = 6 }`  (PRESENT)

## `mut_outer`  (`ts::MutOuter`, `@mutable`)
- `tag`  (`@id(10) long`)               = `9`
- `leaf` (`@id(20) MutLeaf`, `@mutable`)= `{ u = 100, v = 1.25 }`
- `list` (`@id(30) sequence<MutLeaf>`)  = `[ { u = 1, v = 0.5 }, { u = 2, v = 0.25 } ]`

`MutLeaf` (`@mutable`): `@id(1) u (long)`, `@id(2) v (double)`.
