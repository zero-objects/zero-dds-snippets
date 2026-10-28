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
across all five stacks (see [`fastdds/README.md`](fastdds/README.md) for why the
extensibility annotation matters).

## Interop matrix (verified live)

Host: single Linux box, RTPS-UDP, topic `Square`. Each `run.sh` uses a domain of
its own (Cyclone 70, Fast DDS 71, OpenDDS 72, OpenSplice 73, RTI 91; `DOMAIN`
overrides it) and
aborts if that domain's port band is in use: a DDS domain is a UDP port band
(RTPS 2.5 §9.6.1.1), so a run on domain 0 counts whatever else is on the host as
its own result.

| Peer | Vendor → ZeroDDS sub | ZeroDDS pub → Vendor | Notes |
|---|:---:|:---:|---|
| **CycloneDDS 11.0.1** | ✅ 69 | ✅ 59 | Cyclone's writer defaults to **XCDR1** for `@final` — the case that motivated the `DataRepresentation` reader fix (Bug DR1). |
| **Fast DDS 3.x** | ✅ 46 | ✅ 58 | Strictest XTypes matcher: the reader must opt into XCDR2 (`INCOMPATIBLE_QOS` policy 23 otherwise) — the mirror of Bug DR1. See `fastdds/`. |
| **OpenDDS 3.34** | ✅ 68 | ✅ 52 | A late-joining OpenDDS writer's first sample to us is mid-stream; a best-effort ZeroDDS reader must skip the leading SN gap (ZeroDDS Bug BE-GAP). Its IDL writes `@must_understand` on the key, which opendds_idl does not set by itself. See `opendds/`. |
| **OpenSplice CE 6.9** | ✅ 58 | ✅ 59 | No XTypes: `idlpp` reads keys through `#pragma keylist` only and the product writes XCDR1 only, so its peer is built from a keylist copy of the type. The domain comes from the configuration file `OSPL_URI` names. See `opensplice/`. |
| **RTI Connext 7.7** | ✅ 11 | ✅ 40 | `rtiddsgen` console pub/sub. The RTI reader needs XCDR2 added to its representation (same as Fast DDS), the two profile names the generated example looks up have to keep their names, and `rtiddsgen` needs `-unboundedSupport` or it bounds the key at 255. See `rti/`. |

✅ = verified live on one host, both directions, real samples decoded
correctly; the number is the received-sample count of the last run of
`ci/run-examples.sh` on the Linux test host.

**All five DDS stacks interoperate with ZeroDDS in both directions.**
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

The ZeroDDS side is [`zerodds/`](zerodds): `shapes_pub` and `shapes_sub`, with
the type generated from the same [`ShapeType.idl`](ShapeType.idl) the peers
use. Build it once:

```sh
(cd zerodds && cargo build --release)
```

Each vendor directory has a `run.sh` that builds the vendor's peer the first
time, runs both directions and prints the received-sample counts:

```sh
bash cyclone/run.sh     # or fastdds/, opendds/, rti/
```

`ci/run-examples.sh` runs the five legs as the rows `cross-vendor-shapes/cyclone`,
`/fastdds`, `/opendds`, `/opensplice` and `/rti`, each after checking that the vendor's
toolchain is on the host. The per-vendor `README.md` names what that is.

## Reference

- No page on the website explains this example yet; this README is the whole of it.
- This example in the repository: [zero-dds-snippets/cross-vendor-shapes](https://github.com/zero-objects/zero-dds-snippets/tree/main/cross-vendor-shapes)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
