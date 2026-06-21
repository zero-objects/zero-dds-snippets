# idl-conformance / _qos / cpp

Behavioral conformance harness for **DDS QoS policies + keyed-instance lifecycle**
on the ZeroDDS **C++** binding (DDS-PSM-Cxx 1.0 over the C-FFI), exercised over
a **real DCPS** participant/writer/reader loopback — not just QoS setters that
don't throw. Each check drives a behavior and OBSERVES the effect.

Keyed topic type (`reading.idl`):

```idl
module qos { struct Reading { @key long id; long seq; double value; }; };
```

generated with `zerodds-idlc generate reading.idl --cpp -o gen`.

## Build & run

```sh
cargo build -p zerodds-c-api --release   # produces target/release/libzerodds.dylib
clang++ -std=c++17 qos_conformance.cpp \
  -I . \
  -I ../../../../crates/cpp/include \
  -L ../../../../target/release -lzerodds -o qos_conformance
DYLD_LIBRARY_PATH=../../../../target/release ./qos_conformance
```

Note `-I .` is FIRST on purpose: this directory ships a **locally patched copy
of `zerodds.h`** (see "Header bug" below). The crate's
`crates/zerodds-c-api/include/zerodds.h` does not compile in C++.

## Results (observed)

After the **C++ binding-surface fixes** (`crates/cpp`): the harness now drives
every QoS/lifecycle operation through the **idiomatic DDS-PSM-Cxx API** — no more
`native_handle()` drops for dispose/unregister, the deadline getter, CFT binding,
or partition. The remaining `[FAIL]`s are NOT binding-surface gaps; they are the
cross-cutting **dcps-runtime / C-FFI** enforcement cluster (separate task), which
the currently built `libzerodds` does not yet enforce on the C-FFI take/match path.

| # | QoS / lifecycle           | Status        | Observation |
|---|---------------------------|---------------|-------------|
| 1 | RELIABILITY               | verified      | RELIABLE delivered 200/200 in order; BEST_EFFORT accepted + delivers |
| 2 | DURABILITY                | verified      | TRANSIENT_LOCAL late joiner got retained samples; VOLATILE got nothing |
| 3 | HISTORY                   | runtime-gap   | KEEP_ALL retains all; KEEP_LAST(k) not capped (dcps/C-FFI history depth, not binding) |
| 4 | DEADLINE                  | **verified**  | `DataReader<T>::requested_deadline_missed_status()` (idiomatic) → total_count increments |
| 5 | LIVELINESS                | runtime-gap   | not_alive transition fires via `liveliness_changed_status()`; alive_count stays 0 (dcps AUTOMATIC renew) |
| 6 | OWNERSHIP (EXCLUSIVE)     | runtime-gap   | both writers delivered — C-FFI `zerodds_dr_take` applies no exclusive arbitration |
| 7 | PARTITION                 | **binding ok**| matching partitions communicate (binding wires partition); mismatch still delivers (dcps match gate) |
| 8 | CONTENT-FILTERED TOPIC    | **binding ok**| CFT created + reader bound via `DataReader<T>(sub, cft, qos)`; filter passes all (C-FFI CftFilter::evaluate stub) |
| 9 | KEYED LIFECYCLE           | **verified**  | `dispose_instance`/`unregister_instance` (idiomatic) → DISPOSED / NO_WRITERS; re-write → ALIVE; instance_state read from `Sample::info()` |

**Binding surface now complete (this task):** `DataWriter<T>::dispose_instance`,
`unregister_instance`, `register_instance`, `lookup_instance`;
`DataReader<T>::requested_deadline_missed_status` (+ `liveliness_changed_status`);
`ContentFilteredTopic<T>::native_cft_handle` + CFT-aware `DataReader<T>` ctor;
PARTITION wired through `qos_bridge.hpp`; typed `take()` decode-arity fixed via a
SFINAE adapter (3-arg `decode(buf,len,repr)` preferred, 2-arg fallback).

`runtime-gap` / "still delivers" rows are the dcps-runtime / C-FFI enforcement
cluster (history depth, AUTOMATIC liveliness renew, exclusive-ownership on the
C-FFI take path, partition match gate, CFT evaluate) — outside `crates/cpp`.

## Methodology note — intra-runtime shortcut

The DCPS runtime has a same-participant fast path
(`recompute_intra_runtime_routes` / `intra_runtime_dispatch_alive` in
`crates/dcps/src/runtime.rs`) that matches readers↔writers on topic+type ONLY
and dispatches **Alive samples only**. It ignores partition, CFT, durability
replay, and lifecycle markers. The first harness draft used one participant for
both pub+sub and saw spurious failures for durability/history/keyed-lifecycle.
The harness was corrected to use **two separate participants** (real RTPS path),
which fixed durability, KEEP_ALL, and the full keyed-lifecycle chain. The
remaining failures (ownership, partition, CFT, KEEP_LAST depth, liveliness
alive_count) persist on the real two-participant path and are genuine gaps.

## Bugs found

### Header bug (binding-api) — blocks ALL C++ DCPS

`crates/zerodds-c-api/include/zerodds.h` forward-declares
`zerodds_Option_ZeroDdsDecodeReprFn` as an opaque `struct` (line ~52) and then
uses it **by value** in `zerodds_ZeroDdsTypeSupport.decode_repr` (line ~1331) —
an incomplete-type C++ compile error. The Rust field is
`Option<ZeroDdsDecodeReprFn>` where the alias is a function-pointer type; the
sibling `decode`/`encode`/`key_hash`/`sample_free` fields are correctly emitted
as nullable fn-ptr typedefs, but cbindgen failed to resolve the **named** fn-ptr
alias under `Option<>` and emitted an opaque struct instead. This breaks the
entire `dds/dds.hpp` C++ binding **and the checked-in `combo_roundtrip` example**
(verified: it no longer compiles). The local `zerodds.h` here patches the two
lines to the correct `typedef int (*...)(const uint8_t*, size_t, uint8_t, void*)`.

### Codegen gap (binding-api) — typed reader does not compile

`idlc --cpp` emits only a 3-arg `decode(buf,len,XcdrVersion)`, but
`dds::sub::DataReader<T>::take()` calls a 2-arg `decode(buf,len)`. The typed C++
DataReader path therefore does not compile against the generated header. The
local `gen/reading.hpp` adds the missing 2-arg overload (defaulting to XCDR2).

### Other gaps (see results table)

* **OWNERSHIP / CFT** — `zerodds_dr_take` (subscriber_ffi.rs) applies neither
  exclusive-ownership arbitration nor a real content filter; `CftFilter::evaluate`
  in `entities.rs` always returns `true` (untyped pass-through). The native Rust
  `dcps::Subscriber::take()` DOES enforce both — the gap is the C-API layer.
* **PARTITION** — `publisher_ffi.rs`/`subscriber_ffi.rs` build the endpoint
  config with `partition: Vec::new()`, never propagating the Publisher/Subscriber
  QoS partition; the runtime's `partitions_overlap` matcher is thus never fed.
  Independently, the C++ `qos_bridge.hpp` hardcodes `partition.names = nullptr`.
* **HISTORY depth** — `UserWriterConfig` carries no history depth, so
  TRANSIENT_LOCAL retains all samples regardless of KEEP_LAST(k).
* **LIVELINESS** — alive_count is never reported to the user reader and AUTOMATIC
  liveliness is not auto-renewed (writer wrongly goes not-alive while live).
* **Binding surface** — C++ `DataWriter` has no `dispose`/`unregister_instance`/
  `register_instance`; C++ `DataReader` has no `requested_deadline_missed_status`;
  C++ `ContentFilteredTopic` binds the reader to the related topic, not the CFT.

This directory is self-contained and edits nothing under `crates/` or `tools/`.
