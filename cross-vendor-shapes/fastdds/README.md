# ZeroDDS ↔ eProsima Fast DDS

Live RTPS interop on the ShapesDemo `Square` topic using a Fast DDS C++ peer.

```
=== A) Fast DDS pub -> ZeroDDS sub ===
ZeroDDS received from Fast DDS: 49
=== B) ZeroDDS pub -> Fast DDS sub ===
Fast DDS received from ZeroDDS: 40
  BLUE Shape with size 30 at X:120, Y:225 RECEIVED
RESULT: PASS (both directions)
```

**Status:** verified live **both directions** against Fast DDS 3.x — once the
reader is told to accept both wire representations (see step 2 below). That one
QoS line is the whole story of this project: Fast DDS is the strictest XTypes
matcher, and getting it right is the proof we understand its defaults.

## The peer: Fast DDS `topic_instances` example, regenerated `@final`

Fast DDS ships a ShapeType console pub/sub in
`examples/cpp/topic_instances`. Two adjustments make it interoperate with the
canonical ShapesDemo wire form (and reveal exactly how Fast DDS differs):

1. **Extensibility.** The upstream example declares the type
   `@extensibility(APPENDABLE)`. CycloneDDS, RTI ShapesDemo and ZeroDDS use
   **`@final`**. These two *do* match — Fast DDS reports no incompatible QoS — but
   they have **different wire forms**: `@final` is `PLAIN_CDR2` (no DHEADER) while
   `@appendable` is delimited `D_CDR2` (with a per-sample DHEADER). So an
   `@appendable` reader matches the ZeroDDS `@final` writer yet decodes **zero**
   of its samples. *(Verified live: `@appendable` reader → `matched=1`, 0 received;
   `@final` reader → 40 received.)* We therefore regenerate the type from the
   `@final` [`ShapeType.idl`](ShapeType.idl) in this directory:

   ```sh
   fastddsgen -replace ShapeType.idl     # or: java -jar fastddsgen.jar -replace ShapeType.idl
   ```

   This rewrites `ShapeType*.{hpp,cxx,ipp}`. Copy them over the example's
   generated files (keep the example's `*App.cpp`, `CLIParser.hpp`, `main.cpp`,
   `CMakeLists.txt`).

2. **QoS.** The example sets `history().depth = sample_limit`, and `sample_limit`
   defaults to `0` — which Fast DDS rejects (`KEEP_LAST` needs depth > 0). Pass
   `-s <N>` (e.g. `-s 50`), or clamp the depth in `PublisherApp.cpp` /
   `SubscriberApp.cpp`:

   ```cpp
   writer_qos.history().depth = (sample_limit > 0) ? sample_limit : 10;
   ```

   The subscriber also defaults to `TRANSIENT_LOCAL` durability; a plain
   ShapesDemo publisher is `VOLATILE`, and a `TRANSIENT_LOCAL` reader will not
   match a `VOLATILE` writer (RxO: reader request ≤ writer offer). Set the reader
   to `VOLATILE` for the ZeroDDS-publisher direction.

   **Data representation (the key one).** An empty `DataRepresentationQosPolicy`
   on a reader defaults to **XCDR1 only** (XTypes 1.3 §7.6.2 — an empty list
   *is* `{XCDR_DATA_REPRESENTATION}`). ZeroDDS's `@final` writer sends **XCDR2**,
   so the stock Fast DDS reader rejects it with `INCOMPATIBLE_QOS`
   `last_policy_id = 23` (DATA_REPRESENTATION) and never matches. CycloneDDS's
   reader is lenient and accepts both; Fast DDS follows the strict literal. Make
   the reader accept both — what a robust ShapesDemo reader does anyway:

   ```cpp
   reader_qos.representation().m_value.push_back(DataRepresentationId_t::XCDR_DATA_REPRESENTATION);
   reader_qos.representation().m_value.push_back(DataRepresentationId_t::XCDR2_DATA_REPRESENTATION);
   ```

   This is the mirror image of ZeroDDS's own Bug DR1 (where *our* reader had to
   learn to accept both) — the same XTypes rule, seen from the other side.

## Build

```sh
cd <fast-dds>/examples/cpp/topic_instances
# (after dropping in the @final-regenerated ShapeType* files + QoS tweaks)
cmake -B build -DCMAKE_PREFIX_PATH=<fastdds-install> -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Verified against **Fast DDS 3.x** (`fastddsgen` from the matching release).

## Run

ZeroDDS shapes examples first:

```sh
cargo build -p zerodds-dcps --example shapes_demo_publisher --example shapes_demo_subscriber
```

Fast DDS publisher → ZeroDDS subscriber (the proven direction):

```sh
./../../../target/debug/examples/shapes_demo_subscriber Square 0 &
LD_LIBRARY_PATH=<fastdds-install>/lib \
  ./build/topic_instances publisher -d 0 -n Square -i 1 -s 50
```

`run.sh` automates both directions; set `FASTDDS_BIN` and `FASTDDS_LIB`.

## What we learned about Fast DDS

Fast DDS is the strictest XTypes matcher of the four stacks here, and each of the
three tweaks above is a place where its defaults differ from the de-facto
ShapesDemo wire form:

| Tweak | Fast DDS default | What interoperates | How it fails otherwise (verified) |
|---|---|---|---|
| Extensibility | `@appendable` (this example) | `@final` | matches, but **0 samples** (D_CDR2 DHEADER vs PLAIN_CDR2) |
| Reader durability | `TRANSIENT_LOCAL` | `VOLATILE` | `INCOMPATIBLE_QOS` **policy 2** (DURABILITY), no match |
| Reader representation | empty ⇒ `{XCDR1}` | `{XCDR1, XCDR2}` | `INCOMPATIBLE_QOS` **policy 23** (DATA_REPRESENTATION), no match |

Each "how it fails" column is observed, not assumed: the QoS policy ids come from
the reader's `on_requested_incompatible_qos` callback, the extensibility row from
a `matched=1 / 0-received` run. None of these are ZeroDDS bugs — they are
reader/QoS configuration that any cross-vendor Fast DDS deployment has to get
right. Pinning them down *is* the understanding this project demonstrates.

Part of **[ZeroDDS](https://zerodds.de)**.
