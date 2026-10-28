# ZeroDDS ↔ eProsima Fast DDS

Live RTPS interop on the ShapesDemo `Square` topic using a Fast DDS C++ peer.

```
=== A) Fast DDS pub -> ZeroDDS sub ===
ZeroDDS received from Fast DDS: 46
=== B) ZeroDDS pub -> Fast DDS sub ===
Fast DDS received from ZeroDDS: 58
RESULT: PASS (both directions; A=46 B=58)
```

## The peer

[`pub.cxx`](pub.cxx) and [`sub.cxx`](sub.cxx), with the type support
`fastddsgen` generates from the `@final` [`../ShapeType.idl`](../ShapeType.idl)
and built with [`CMakeLists.txt`](CMakeLists.txt). `run.sh` does the generation
and the build the first time, into `build/`.

Both endpoints are `RELIABLE` and `VOLATILE`. The reader lists both data
representations:

```cpp
rq.representation().m_value.push_back(XCDR_DATA_REPRESENTATION);
rq.representation().m_value.push_back(XCDR2_DATA_REPRESENTATION);
```

An empty `DataRepresentationQosPolicy` on a reader is **XCDR1 only** (XTypes
1.3 §7.6.2: an empty list *is* `{XCDR_DATA_REPRESENTATION}`). A ZeroDDS writer
that sends XCDR2 is then refused with `INCOMPATIBLE_QOS`
`last_policy_id = 23` (DATA_REPRESENTATION) and never matches. Cyclone DDS's
reader accepts both; Fast DDS follows the literal rule. This is the mirror image
of ZeroDDS's own Bug DR1 (where *our* reader had to learn to accept both).

## Run

```sh
(cd ../zerodds && cargo build --release)
bash run.sh            # domain 71; DOMAIN, FASTDDS_HOME and FASTDDSGEN override
```

Needs Fast DDS 3 under `FASTDDS_HOME` (default `/opt/fastdds`), `fastddsgen` on
`PATH` and cmake.

## What we learned about Fast DDS

An earlier version of this leg used Fast DDS's own ShapeType console example,
`examples/cpp/topic_instances`. Three of its defaults differ from the
de-facto ShapesDemo wire form, and each failed in its own way:

| Tweak | Fast DDS default | What interoperates | How it fails otherwise (verified) |
|---|---|---|---|
| Extensibility | `@appendable` (`topic_instances`) | `@final` | matches, but **0 samples** (D_CDR2 DHEADER vs PLAIN_CDR2) |
| Reader durability | `TRANSIENT_LOCAL` | `VOLATILE` | `INCOMPATIBLE_QOS` **policy 2** (DURABILITY), no match |
| Reader representation | empty ⇒ `{XCDR1}` | `{XCDR1, XCDR2}` | `INCOMPATIBLE_QOS` **policy 23** (DATA_REPRESENTATION), no match |

Each "how it fails" column is observed, not assumed: the QoS policy ids come from
the reader's `on_requested_incompatible_qos` callback, the extensibility row from
a `matched=1 / 0-received` run. None of these are ZeroDDS bugs — they are
reader/QoS configuration that any cross-vendor Fast DDS deployment has to get
right. Pinning them down *is* the understanding this project demonstrates.

Part of **[ZeroDDS](https://zerodds.de)**.
