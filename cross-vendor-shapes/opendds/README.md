# ZeroDDS ↔ OpenDDS

Live RTPS interop on the ShapesDemo `Square` topic, **both directions**, with a
small OpenDDS C++ console publisher/subscriber.

```
=== A) OpenDDS pub -> ZeroDDS sub ===
ZeroDDS received from OpenDDS: 68
=== B) ZeroDDS pub -> OpenDDS sub ===
OpenDDS received from ZeroDDS: 52
RESULT: PASS (both directions; A=68 B=52)
```

## What this uncovered — ZeroDDS Bug BE-GAP

Direction A was the harder one and exposed a **real ZeroDDS conformance bug**.

An OpenDDS writer publishes into its own history immediately; by the time the
ZeroDDS reader associates, the writer's first sample *to us* is already
mid-stream (e.g. SN 13, not SN 1). ZeroDDS's reader **matched** and **accepted**
those samples — but never delivered them to the application: it kept them in the
receive cache waiting to fill the leading gap (SN 1..12), which a **best-effort**
reader never repairs (it does not NACK). Result: deadlock, zero samples.

Per RTPS §8.4.12.1 a best-effort reader makes no in-order-without-loss guarantee
and must deliver from the lowest received SN. ZeroDDS now does — see the main
repo's `crates/rtps/src/reliable_reader.rs` (`best_effort` gap-skip) and the
`best_effort_reader_skips_leading_gap` regression test. CycloneDDS/Fast DDS hid
this because their writers happened to start at SN 1.

## Setup

OpenDDS with RTPS, built from source (the `OpenDDS_ROOT`'s `setenv.sh` puts
`opendds_idl`, ACE/TAO and the libs on the path). This suite was verified
against **OpenDDS 3.34**.

> Note: if your OpenDDS tree was configured `--security`, build against that same
> tree — a security-*enabled* headers / security-*disabled* libs mix fails to
> compile the generated `TypeSupportImpl` (`SecurityConfig::get_access_control`
> missing). Using the consistent build resolves it.

```sh
source <opendds>/setenv.sh
cmake -B build -DOpenDDS_ROOT=<opendds>
cmake --build build -j
```

`CMakeLists.txt`, `publisher.cpp`, `subscriber.cpp`, [`ShapeType.idl`](ShapeType.idl)
(`@topic @final`, type name **`ShapeType`** — global scope, no module, so the
wire type name matches the other vendors) and [`rtps.ini`](rtps.ini)
(`DCPSDefaultDiscovery=DEFAULT_RTPS`) are all in this directory.

## The key member carries `@must_understand`

XTypes 1.3 §7.2.2.4.4.4.8 gives every key member the must-understand flag.
ZeroDDS and Cyclone DDS set IS_MUST_UNDERSTAND on a key member without being
told; opendds_idl 3.34 sets it only when `@must_understand` is written, so the
two minimal TypeObjects differ in that flag and so do their hashes. OpenDDS
3.34 also numbers TypeConsistencyKind from 1 (`DISALLOW_TYPE_COERCION = 1`,
`ALLOW_TYPE_COERCION = 2`) where XTypes 1.3 numbers it from 0, reads a remote
reader's ALLOW_TYPE_COERCION as DISALLOW and then matches identical type
identifiers only. With the flag written out, OpenDDS generates the TypeObject
the specification asks for and the identifiers agree. The comment in
[`ShapeType.idl`](ShapeType.idl) names the OpenDDS sources.

## Run

```sh
(cd ../zerodds && cargo build --release)
DDS_ROOT=<opendds> bash run.sh      # domain 72; DOMAIN overrides
```

`run.sh` builds the peer into `build/` the first time. By hand:

```sh
# A) OpenDDS pub -> ZeroDDS sub
../zerodds/target/release/shapes_sub Square 72 &
./build/cmake/opendds_pub -DCPSConfigFile rtps.ini 72

# B) ZeroDDS pub -> OpenDDS sub
./build/cmake/opendds_sub -DCPSConfigFile rtps.ini 72 &
../zerodds/target/release/shapes_pub Square BLUE 72
```

The publisher waits for a matched subscriber before its first `write()` so the
demo is clean; the BE-GAP fix means it works even without that wait.

Part of **[ZeroDDS](https://zerodds.de)**.
