# RTI Connext DDS — rich-type pub/sub (C++11)

`rtiddsgen` emits the type support from [`Rich.idl`](Rich.idl) (three structs).
Use `-unboundedSupport` so the `sequence<octet>` is unbounded (RTI otherwise
defaults it to a 100-element bound); the templated `Rich_publisher` /
`Rich_subscriber` use the modern C++ API and dispatch on the extensibility.

```sh
export NDDSHOME=<connext-install>
$NDDSHOME/bin/rtiddsgen -language C++11 -platform x64Linux4gcc8.5.0 \
    -unboundedSupport -replace -d . Rich.idl
make -f makefile_Rich_x64Linux4gcc8.5.0
```

Run (argument 2 = `f` / `a` / `m`):

```sh
./objs/x64Linux4gcc8.5.0/Rich_subscriber 0 m
./objs/x64Linux4gcc8.5.0/Rich_publisher  0 m 50
```

## Two things to configure

[`USER_QOS_PROFILES.xml`](USER_QOS_PROFILES.xml) forces **`<transport_builtin>`
`UDPv4`**: same-host, RTI prefers the shared-memory transport, which a non-RTI peer
cannot read — UDPv4 keeps the traffic on the wire. The reader also opts into both
data representations and best-effort reliability.

## What this shows

RTI is **XCDR1-native** like Fast DDS, but encodes its `@mutable` sequence member
with the **PID_EXTENDED long form** carrying the MUST_UNDERSTAND flag (`0x7F01`),
where Fast DDS uses the short parameter id. ZeroDDS's PL_CDR1 decoder masks the
RTPS flag bits (`& 0x3FFF`) so it accepts both.

## What this currently measures: nothing gets through

`run.sh` runs all six cells — `@final` / `@appendable` / `@mutable` × both
directions, each on a domain of its own — and every one of them reports **0
samples**. RTI does not match the ZeroDDS endpoint at all; it refuses it during
discovery, before any data:

```
PRESTypeObjectV2Table_assertTypeObjectRecord:BAD PARAMETER FAILURE
  | TypeObjectV2 serializedSize mismatch (current: 111, new: 107)
```

Both stacks announce the *same* COMPLETE `TypeIdentifier` for `RichF`
(`f2 c1 64 bf d8 e6 94 4d c9 ee 5e 2e 5f 5c 20 00`) and a *different*
`typeobject_serialized_size` beside it — 107 from ZeroDDS, 111 from RTI. See
`../README.md`. The manifest row
`cross-vendor-rich/rti` is `blocked` on this and carries `run.sh` as the command
the defect stops, so the harness reports `BLOCKER STALE` the day it passes.

## Run it

```sh
(cd ../zerodds && cargo build --release)
bash run.sh              # DOMAIN_BASE=92 by default, six consecutive domains
```

`run.sh` generates the type support with `rtiddsgen`, builds the peer, checks
that the six RTPS port bands are free before it measures anything, and prints
the sample count of every cell.

Part of **[ZeroDDS](https://zerodds.de)**.
