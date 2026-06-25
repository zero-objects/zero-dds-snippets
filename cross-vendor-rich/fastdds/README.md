# eProsima Fast DDS — rich-type pub/sub (C++)

Generate the type support from [`Rich.idl`](Rich.idl) (three structs) with
`fastddsgen`, then build the templated `pub` / `sub`:

```sh
fastddsgen -replace Rich.idl        # emits Rich*.hpp/.cxx
cmake -B build -DCMAKE_PREFIX_PATH=<fastdds-install> -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Run (argument 2 = `f` / `a` / `m`):

```sh
./build/rich_fd_sub 0 m
./build/rich_fd_pub 0 m 50
```

## What this shows

Fast DDS is **XCDR1-native**: `@final` and `@appendable` both come out as plain
CDR (`0x0001`, no DHEADER) and `@mutable` as PL_CDR (`0x0003`). The reader opts
into both representations (`XCDR_DATA_REPRESENTATION` + `XCDR2_DATA_REPRESENTATION`)
so it also accepts ZeroDDS's XCDR2 output. Both directions decode `integrity=OK`.

Part of **[ZeroDDS](https://zerodds.de)**.
