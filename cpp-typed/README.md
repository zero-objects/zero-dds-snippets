# cpp-typed

A C++ program (`typed.cpp`) that publishes and receives a strongly-typed IDL
struct instead of raw bytes. From `temperature.idl` the idlc-cpp codegen emits
`gen/sensor_msgs/msg/Temperature.hpp` with a field-order constructor and
`celsius()`/`sensor_id()` accessors. The demo uses
`zerodds::TypedWriter<Temperature>` / `TypedReader<Temperature>` to brace-init
`Temperature t{23, "A7"}`, `write(t)`, and round-trip it back through `r.take(got)`
— proving the generated type marshals through DDS and decodes to the same fields.

## Build & run

Build the C-API cdylib first, from the ZeroDDS checkout:

```sh
cargo build -p zerodds-c-api --release
```

The generated header in `gen/` is checked in. To regenerate it from the IDL:

```sh
cargo run -p zerodds-idl-cpp -- temperature.idl --out gen
```

Then compile against the wrapper headers plus this demo's `gen/` directory:

```sh
g++ -std=c++17 typed.cpp \
    -I . \
    -I /path/to/zerodds/crates/cpp/include \
    -I /path/to/zerodds/crates/zerodds-c-api/include \
    -L /path/to/zerodds/target/release -lzerodds \
    -o typed
LD_LIBRARY_PATH=/path/to/zerodds/target/release ./typed
```

## Reference

This is the runnable companion to the code snippet on
**[C++ bindings](https://zerodds.de/bindings/cpp/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
