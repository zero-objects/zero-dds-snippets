# ZeroDDS ↔ OpenDDS

Live RTPS interop on the ShapesDemo `Square` topic, **both directions**, with a
small OpenDDS C++ console publisher/subscriber.

```
A) OpenDDS pub -> ZeroDDS sub:   ZeroDDS received: 35   <- color=GREEN x=120 y=225 size=30
B) ZeroDDS pub -> OpenDDS sub:   OpenDDS received: 63   <- color=BLUE  x=120 y=225 RECEIVED
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

## Run

```sh
cargo build -p zerodds-dcps --example shapes_demo_publisher --example shapes_demo_subscriber

# A) OpenDDS pub -> ZeroDDS sub
./../../../target/debug/examples/shapes_demo_subscriber Square 0 &
./build/opendds_pub -DCPSConfigFile rtps.ini

# B) ZeroDDS pub -> OpenDDS sub
./build/opendds_sub -DCPSConfigFile rtps.ini &
./../../../target/debug/examples/shapes_demo_publisher Square BLUE 0
```

The publisher waits for a matched subscriber before its first `write()` so the
demo is clean; the BE-GAP fix means it works even without that wait.

Part of **[ZeroDDS](https://zerodds.de)**.
