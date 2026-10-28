# ZeroDDS ↔ Eclipse CycloneDDS

Live RTPS interop on the ShapesDemo `Square` topic, **both directions**, using
CycloneDDS's own Python binding as the peer.

```
=== A) Cyclone DDS pub -> ZeroDDS sub ===
ZeroDDS received from Cyclone: 69
=== B) ZeroDDS pub -> Cyclone DDS sub ===
Cyclone received from ZeroDDS: 59
RESULT: PASS (both directions; A=69 B=59)
```

## What this exercises

CycloneDDS's IDL compiler defaults a plain struct to **`@final`**, and a `@final`
writer defaults to the **XCDR1** data representation (for backward compatibility
with non-XTypes peers). Direction A therefore puts an XCDR1 writer in front of
the ZeroDDS reader — the exact case that requires a reader to accept *both* XCDR1
and XCDR2 (OMG XTypes 1.3 §7.6.2). See
`docs/interop/data-representation-cross-vendor.md` in the main repo.

## Setup

1. **CycloneDDS + Python binding.** Either install a release or build from
   source, then install the binding:

   ```sh
   # using a CycloneDDS install at $CYCLONEDDS_HOME
   export CYCLONEDDS_HOME=/opt/cyclonedds
   export LD_LIBRARY_PATH=$CYCLONEDDS_HOME/lib:$LD_LIBRARY_PATH
   pip install cyclonedds
   ```

   This suite was verified against **CycloneDDS 11.0.1** / `cyclonedds` 11.x.
   Note the 11.x API specifics already encoded in the scripts: the key is the
   `@keylist([...])` decorator, `reader.take(N=100)` uses a capital `N`, and
   `from __future__ import annotations` must **not** be present (it breaks the
   idl type resolver).

2. **The ZeroDDS side**, generated from the same `ShapeType.idl`:

   ```sh
   (cd ../zerodds && cargo build --release)
   ```

3. **Multicast.** SPDP discovery uses IP multicast. On a single host over
   loopback, enable multicast on `lo` if needed: `ip link set lo multicast on`.
   On a normal LAN interface nothing special is required.

## Run

```sh
bash run.sh            # domain 70; DOMAIN and PYTHON override
```

Or drive the pieces by hand — e.g. CycloneDDS publisher → ZeroDDS subscriber:

```sh
../zerodds/target/release/shapes_sub Square 70 &
python3 shapes_pub.py Square GREEN 70
```

Part of **[ZeroDDS](https://zerodds.de)**.
