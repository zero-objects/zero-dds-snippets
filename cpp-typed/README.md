# cpp-typed

A C++ program (`typed.cpp`) that publishes and receives a strongly-typed IDL
struct instead of raw bytes. From `temperature.idl`, `zerodds-idlc generate
--lang c++` emits `gen/temperature.hpp`: the declaration as an aggregate with
its members in IDL order, the XCDR2 codec, and the
`dds::topic::topic_type_support<Temperature>` specialisation that
`zerodds/dds.hpp` asks every topic type for. The demo uses
`zerodds::TypedWriter<Temperature>` / `TypedReader<Temperature>` to brace-init
`Temperature t{23, "A7"}`, `write(t)`, and round-trip it back through `r.take(got)`
— proving the generated type marshals through DDS and decodes to the same fields.

## Build & run

Build the C-API cdylib and the C++ wire runtime first, from the ZeroDDS
checkout. The generated codec calls the second one; it is four translation
units and a `Makefile`, with no build system beyond that:

```sh
cargo build -p zerodds-c-api --release
make -C runtime/cpp build/libzerodds_wire.a
```

The generated header in `gen/` is checked in. To regenerate it from the IDL:

```sh
cargo run -p zerodds-idlc -- generate --lang c++ temperature.idl --out gen
```

Then compile against the wrapper headers, the wire runtime and this demo's
`gen/` directory:

```sh
g++ -std=c++17 typed.cpp \
    -I . \
    -I /path/to/zerodds/crates/cpp/include \
    -I /path/to/zerodds/runtime/cpp/include \
    -I /path/to/zerodds/crates/zerodds-c-api/include \
    /path/to/zerodds/runtime/cpp/build/libzerodds_wire.a \
    -L /path/to/zerodds/target/release -lzerodds \
    -o typed
LD_LIBRARY_PATH=/path/to/zerodds/target/release ./typed
```

It prints:

```
got: celsius=23 sensor_id=A7
```

## Reference

- Explained on the website: [C++](https://zerodds.org/bindings/cpp/)
- This example in the repository: [zero-dds-snippets/cpp-typed](https://github.com/zero-objects/zero-dds-snippets/tree/main/cpp-typed)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
