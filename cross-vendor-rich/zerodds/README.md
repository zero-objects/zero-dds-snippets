# ZeroDDS — rich-type pub/sub

A standalone Rust crate. `build.rs` runs the IDL→Rust codegen on
[`Rich.idl`](Rich.idl) (all three extensibilities), and `rich_pub` / `rich_sub`
pick one by a CLI argument.

```sh
cargo build --release
```

Run (argument 2 selects the extensibility: `f` = `@final`, `a` = `@appendable`,
`m` = `@mutable`):

```sh
# subscriber on domain 0, @mutable topic
./target/release/rich_sub 0 m

# publisher: domain 0, @mutable, 50-byte blob
./target/release/rich_pub 0 m 50
```

## Environment knobs

| env | effect |
|---|---|
| `ZD_BE` (on `rich_sub`) | make the reader **best-effort** instead of reliable — the clean path for a late-joining cross-vendor reader against a volatile writer |
| `ZERODDS_WIRE_BIG_ENDIAN` (on `rich_pub`) | emit **big-endian** samples (encapsulation `*_BE` + `DdsType::encode_be`) instead of the little-endian default |

The blob is filled with `k % 251`; the subscriber prints `integrity=OK` when every
byte round-trips. The writer emits the encapsulation id that honestly matches the
type's extensibility (`@final` → `0x07`, `@appendable` → `0x09`, `@mutable` →
`0x0b`; the `*_BE` variants under `ZERODDS_WIRE_BIG_ENDIAN`).

Part of **[ZeroDDS](https://zerodds.de)**.
