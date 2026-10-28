# idl-conformance / cpp

A standalone, runnable **DCPS** conformance example for the ZeroDDS C++
binding. It writes two fully-populated samples of a multi-feature IDL type
through `zerodds::TypedWriter<T>` over a ZeroDDS participant and takes them
back through `zerodds::TypedReader<T>`, asserting every recovered field equals
what was written — the first with the optional present and the union's
`MODE_ACTIVE` case, the second with the optional absent and the union's
default case.

The type is `combo::Telemetry` from [`combo.idl`](combo.idl); `build.sh`
generates `gen/combo.hpp` from it with `zerodds-idlc generate --lang c++` on
every run (not committed).

## IDL features exercised (all round-trip over DCPS)

One keyed topic type combines:

| Feature                       | IDL                         | Verified field(s)          |
|-------------------------------|-----------------------------|----------------------------|
| `@key` integer                | `@key long unitId`          | `unitId`                   |
| `@key` bounded string         | `@key string<32> region`    | `region`                   |
| enum                          | `combo::Mode mode`          | `mode`                     |
| typedef → primitive           | `CurrentInAmpsType battery` | `batteryCurrent`           |
| sequence of nested struct     | `sequence<Sample> history`  | each `history[i].{seq,value}` |
| nested struct                 | `struct Sample`             | (inside the sequence)      |
| map<string,long>              | `map<string,long> counters` | `counters[ok/err/retry]`   |
| `@optional`                   | `@optional double calibration` | presence + value        |
| fixed array                   | `long window[4]`            | all 4 elements             |
| union (enum discriminator)    | `union Reading switch (Mode)` | `reading._d` + `activeRate` or `faultCode` |
| `@appendable` extensibility   | `struct Telemetry`          | DHEADER framing            |

Each of these is asserted equal after the DCPS roundtrip; the program prints a
`[PASS]`/`[FAIL]` line per field and exits non-zero if any field differs.

## Build & run

```sh
./build.sh
```

`build.sh` builds `zerodds-c-api` and `zerodds-idlc` in the ZeroDDS checkout
this directory sits in (or `$ZERODDS_SRC`), generates `gen/combo.hpp`, builds
`runtime/cpp`'s wire library, compiles `combo_roundtrip.cpp` with
`-Wall -Werror` and runs it on domain 177 (`DOMAIN` overrides it).

## Reference

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS
implementation. Companion to the C++ binding examples
[`cpp-typed`](../../cpp-typed/) and [`cpp-hello`](../../cpp-hello/).
