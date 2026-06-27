<!-- SPDX-License-Identifier: Apache-2.0 -->
# Cross-vendor roundtrip latency — every type, every condition

Where the [README](README.md) proves ZeroDDS speaks every vendor's *wire dialect*,
this companion measures the **full ping/pong round-trip** through the complete
typed (X)CDR + DCPS + RTPS pipeline of each stack — **ZeroDDS, CycloneDDS,
eProsima Fast DDS, OpenDDS, RTI Connext** — under four conditions: plain/simple,
plain/rich, secured/simple, secured/rich.

The benchmark apps live in [`tests/perf/dds-roundtrip-bench`](../../tests/perf/dds-roundtrip-bench)
(one app per vendor, generated from one shared IDL by each vendor's own code
generator). Run on a single Linux host with the five vendor SDKs installed; the
kernel is shared, so absolute µs are **noise-dominated** — read the numbers as
*order-of-magnitude + ranking*, not precise latencies. Pass/fail (does data flow)
is solid.

Two types:
- **simple** — `{ uint32 seq; uint64 t_send_ns; sequence<octet,8192> payload; }`
- **rich** — adds `string<64>`, `double[16]`, `sequence<Waypoint,32>` (each
  `Waypoint` = 2 nested `Vec3` + string + scalars). Maximises the XCDR2
  member-codec work.

## Headline — same-host SHM carrier is now event-driven

ZeroDDS same-host (`zerodds ↔ zerodds`) auto-routes over its POSIX-SHM carrier.
That carrier used to poll the ring with an exponential-backoff sleep, which in a
ping/pong injected a bimodal 66–140 µs round-trip. It is now a **shared futex**
(producer `FUTEX_WAKE`, consumer `FUTEX_WAIT` on the ring head):

| `zerodds ↔ zerodds`, payload 0 | p50 | p90 | p99 |
|---|---|---|---|
| before (sleep-poll) | 66–140 (bimodal) | ~140 | 142 |
| **after (futex)** | **16–27** | **17–29** | **25–35** |

Now the fastest self-cell of the whole matrix — faster than UDP loopback.

## plain / simple (median p50 µs, ping → pong)

payload 0:

| ping\pong | zerodds | cyclone | fastdds | rti |
|---|---|---|---|---|
| **zerodds** | **18.3** | 23.8 | 27.7 | 24.3 |
| cyclone | 35.6 | 34.2 | 51.8 | 42.9 |
| fastdds | 39.0 | 47.1 | 53.0 | 52.9 |
| rti | 37.0 | 22.8 | 49.4 | 44.3 |

## plain / rich (median p50 µs)

| ping\pong | zerodds | cyclone | fastdds | rti | opendds |
|---|---|---|---|---|---|
| **zerodds** | 20 | 41 | 35 | 61 | 60 |
| cyclone | 62 | 70 | 77 | ✗ | ✗ |
| fastdds | 70 | 91 | 64 | 76 | 123 |
| rti | 23 | ✗ | 67 | 56 | ✗ |
| opendds | 103 | ✗ | 79 | ✗ | 226 |

## secured / rich — DDS-Security 1.2, SRTPS (median p50 µs)

| ping\pong | zerodds | cyclone | fastdds¹ | opendds |
|---|---|---|---|---|
| **zerodds** | 91 | 72 | 70 / 28 | 194 |
| cyclone | 113 | 135 | – | ✗ |
| fastdds¹ | 70 / 28 | – | 152 | – |
| opendds | 125 | ✗ | – | 232 |

¹ Fast DDS secured cross-vendor needs RSA identity certs (see below); the cell
shows `zerodds↔fastdds` both directions under RSA.

## The mission — ZeroDDS interoperates with every stack, every condition

| condition | Cyclone | Fast DDS | OpenDDS | RTI |
|---|:--:|:--:|:--:|:--:|
| plain simple | ✓ | ✓ | ✓ | ✓ |
| plain rich | ✓ | ✓ | ✓ | ✓ |
| secured (simple/rich) | ✓ | ✓ | ✓ | n/a² |

² RTI security is a paid add-on, deliberately out of scope.

**Every cell that does *not* light up is a vendor↔vendor incompatibility,
reproducible without ZeroDDS** — ZeroDDS is always the flexible, conformant side:

- **RTI rich** — RTI's XCDR2 serialization of `sequence<non-primitive>` omits the
  spec-mandated collection DHEADER by default (OMG DDS-XTypes 1.3 §7.4.3.5; RTI's
  own release notes call the default non-compliant). ZeroDDS, Cyclone, Fast DDS
  and OpenDDS all emit it. Run RTI with `NDDS_XTYPES_COMPLIANCE_MASK=0x1a9`
  (RTI's documented full-compliance mask) and `zerodds↔rti` / `fastdds↔rti` go
  green. Cyclone↔RTI and OpenDDS↔RTI stay dark — a vendor-pair TypeObject
  mismatch with no ZeroDDS involved.
- **OpenDDS secured** — OpenDDS' AccessControl rejected per-topic
  read/write access-control combined with the permissions file (misreported as
  *"No governance exists for this domain"*, blocking even `opendds↔opendds`).
  Setting those two governance flags to `false` makes one governance acceptable
  to all four stacks; encryption + authentication + participant join control stay
  on.
- **Fast DDS secured** — Fast DDS' PKI-DH handshake rejects EC P-256 identity
  certs cross-vendor (`writer wait_for_matched timeout`). Vendor-side, not
  ZeroDDS: **`cyclone↔fastdds` secured also fails with EC and no ZeroDDS
  involved**, while `fastdds↔fastdds` works with EC. Generate RSA certs and
  `zerodds↔fastdds` secured works both ways. RSA in turn breaks
  `zerodds↔cyclone` (Cyclone prefers EC) — so no single cert algorithm spans all
  four vendors. **ZeroDDS interoperates secured under both EC and RSA.**

Cyclone, Fast DDS and OpenDDS each interoperate (secured) with only *some* of the
others; ZeroDDS is the only stack that interoperates — plain *and* rich *and*
secured — with **all four**.
