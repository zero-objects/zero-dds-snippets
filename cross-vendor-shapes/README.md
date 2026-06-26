# cross-vendor-shapes

Live RTPS interop between **ZeroDDS** and the other DDS implementations, on the
standard OMG **ShapesDemo** type (`ShapeType` / topic `Square`). Each
sub-directory is a self-contained test project that builds the *vendor's own*
publisher and subscriber with the *vendor's own* toolchain and runs them against
ZeroDDS in both directions.

The point is twofold:

1. **Proof.** Real samples cross the wire between two independent stacks — not a
   ZeroDDS-vs-ZeroDDS loopback.
2. **Understanding.** Each `README.md` documents the vendor's build (IDL codegen,
   CMake, QoS) precisely enough to reproduce from a clean checkout. Getting those
   instructions right is itself the evidence that we understand how each stack
   behaves on the wire.

## The shared type

[`ShapeType.idl`](ShapeType.idl) — `@final`, `@key string color`, three `long`s.
`@final` is the canonical ShapesDemo wire form and the one that interoperates
across all four stacks (see [`fastdds/README.md`](fastdds/README.md) for why the
extensibility annotation matters).

## Interop matrix (verified live)

Host: single Linux box, RTPS-UDP, domain 0, topic `Square`.

| Peer | Vendor → ZeroDDS sub | ZeroDDS pub → Vendor | Notes |
|---|:---:|:---:|---|
| **CycloneDDS 11.0.1** | ✅ | ✅ | Cyclone's writer defaults to **XCDR1** for `@final` — the case that motivated the `DataRepresentation` reader fix (Bug DR1). |
| **Fast DDS 3.x** | ✅ | ✅ | Strictest XTypes matcher: the reader must opt into XCDR2 (`INCOMPATIBLE_QOS` policy 23 otherwise) — the mirror of Bug DR1. See `fastdds/`. |
| **OpenDDS 3.34** | ✅ | ✅ | A late-joining OpenDDS writer's first sample to us is mid-stream; a best-effort ZeroDDS reader must skip the leading SN gap (ZeroDDS Bug BE-GAP). See `opendds/`. |
| **RTI Connext 7.7** | ✅ | ✅ | `rtiddsgen` console pub/sub. The RTI reader needs XCDR2 added to its representation (same as Fast DDS). See `rti/`. |

✅ = verified live on one host, both directions, real samples decoded correctly.

**All four mainstream DDS stacks interoperate with ZeroDDS in both directions.**
Two of the cells uncovered real ZeroDDS conformance bugs, now fixed:

- **Bug DR1** — a ZeroDDS *reader* announced only XCDR2 and silently dropped a
  Cyclone/legacy-RTI XCDR1 writer. Fixed: readers accept both representations
  (OMG XTypes 1.3 §7.6.2).
- **Bug BE-GAP** — a ZeroDDS *best-effort reader* blocked delivery forever on a
  leading sequence-number gap (it doesn't NACK to repair it). Fixed: best-effort
  readers skip leading gaps (RTPS §8.4.12.1). This is what unblocked OpenDDS.

The other two cells are vendor reader-side configuration (Fast DDS and RTI
readers must opt into XCDR2) — documented per project as evidence of how each
stack's defaults differ.

## Why this lives next to a `@final` reader fix

CycloneDDS, legacy RTI and OpenDDS < 3.16 default their **writers** to XCDR1 for
`@final` types (pre-XTypes backward compatibility); Fast DDS and modern OpenDDS
default to XCDR2. A reader that announces only one representation silently drops
the other vendor's samples. The spec (OMG XTypes 1.3 §7.6.2) says a default
reader accepts **both** — which is exactly what these projects exercise. See
`docs/interop/data-representation-cross-vendor.md` in the main repo.

## Running

Each project has a `run.sh` that starts the ZeroDDS shapes example and the
vendor pub/sub, runs both directions, and prints the received-sample counts.
Build the ZeroDDS shapes examples first:

```sh
cargo build -p zerodds-dcps --example shapes_demo_publisher --example shapes_demo_subscriber
```

Then see the per-vendor `README.md`.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
