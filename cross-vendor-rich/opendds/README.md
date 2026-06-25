# OpenDDS — rich-type pub/sub (C++)

`opendds_target_sources` runs `opendds_idl` on [`Rich.idl`](Rich.idl) (three
`@topic` structs); the pub/sub dispatch on the extensibility with a token-pasting
macro.

```sh
source <opendds>/setenv.sh
cmake -B build -DOpenDDS_ROOT=<opendds>
cmake --build build -j
```

Run (argument 1 = `f` / `a` / `m`; `-DCPSConfigFile rtps.ini` selects RTPS):

```sh
./build/od_sub m -DCPSConfigFile rtps.ini
./build/od_pub m 50 -DCPSConfigFile rtps.ini
```

## What this shows

OpenDDS is the **strict** stack: its `EncapsulationHeader::to_encoding` rejects a
sample whose encapsulation id does not match the reader type's extensibility. That
makes it the litmus test that ZeroDDS now stamps an *honest* encap id (`@appendable`
→ D_CDR2, `@mutable` → PL_CDR2), not a blanket `@final` header. OpenDDS also sends
a key-only `register_instance` sample per data sample; the ZeroDDS reader skips it
rather than trying to decode it as a full sample. Both directions: `integrity=OK`.

Part of **[ZeroDDS](https://zerodds.de)**.
