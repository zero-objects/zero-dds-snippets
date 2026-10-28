# idl-conformance / _qos / cpp

Behavioral conformance harness for **DDS QoS policies + keyed-instance lifecycle**
on the ZeroDDS **C++** binding (DDS-PSM-Cxx 1.0 over the C-FFI), exercised over
a **real DCPS** participant/writer/reader loopback — not just QoS setters that
don't throw. Each check drives a behavior and OBSERVES the effect.

Keyed topic type (`reading.idl`):

```idl
module qos { struct Reading { @key long id; long seq; double value; }; };
```

## Build & run

```sh
./build.sh
```

`build.sh` builds `zerodds-c-api` and `zerodds-idlc` in the ZeroDDS checkout
this directory sits in (or `$ZERODDS_SRC`), generates `gen/reading.hpp`
(`zerodds-idlc generate --lang c++`, not committed), builds `runtime/cpp`'s
wire library, compiles `qos_conformance.cpp` against `crates/cpp/include`,
`runtime/cpp/include` and `crates/zerodds-c-api/include`, and runs it. The
program prints `[PASS]`/`[FAIL]` per assertion and exits non-zero if any fails.

## Checks

Twenty-four assertions over RELIABILITY, DURABILITY, HISTORY, DEADLINE,
LIVELINESS, OWNERSHIP (EXCLUSIVE), PARTITION (set on the Publisher and the
Subscriber), a ContentFilteredTopic and the keyed instance lifecycle (ALIVE,
NOT_ALIVE_DISPOSED, NOT_ALIVE_NO_WRITERS, ALIVE again on a new sample).

The OWNERSHIP check lets the strength-10 writer write first and counts from
the arrival of its sample: until then the strength-1 writer owns the
instance (DDS 1.4 §2.2.3.23) and its samples are legitimately delivered.

## Open

`LIVELINESS: writer becomes ALIVE (alive_count>0)` fails: the DCPS runtime
counts an AUTOMATIC writer alive only after its first delivered sample and
keeps `alive_count` as a reader-level transition counter, where DDS 1.4
§2.2.4.1 counts the currently alive writers. The remaining 23 assertions pass.
