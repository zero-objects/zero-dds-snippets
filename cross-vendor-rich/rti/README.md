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
where Fast DDS uses the short parameter id. RTI declares that member's length
rounded up to four (DDSI-RTPS 2.5 §9.4.2.11): 56 for the 54 octets of a
50-element `blob`. XTypes 1.3 §7.4.1.2 makes the length the member's own, and a
ZeroDDS codec built in the specification's dialect refuses the sample as a
framing error. `run.sh` therefore builds the ZeroDDS side with
`ZERODDS_RICH_DIALECT=rti`, into `build/zerodds-target`.

All six cells deliver — `@final` / `@appendable` / `@mutable` × both
directions, each on a domain of its own, every sample integrity-checked.

## Run it

```sh
bash run.sh              # DOMAIN_BASE=92 by default, six consecutive domains
```

`run.sh` builds the ZeroDDS side in RTI's dialect, generates the type support
with `rtiddsgen`, builds the peer, checks that the six RTPS port bands are free
before it measures anything, and prints the sample count of every cell.

Part of **[ZeroDDS](https://zerodds.de)**.
