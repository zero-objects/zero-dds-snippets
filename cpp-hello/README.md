# cpp-hello

A minimal C++ program (`hello.cpp`) using the header-only RAII wrapper in
`zerodds/dds.hpp`. It constructs a `zerodds::Runtime`, a reliable
`zerodds::Writer` and `zerodds::Reader` on topic `Chatter` with type
`RawBytes`, waits for matching, writes a `std::vector<uint8_t>` payload, and
reads it back with `r.take()`. It proves the idiomatic C++ surface (constructors
manage lifetimes, `write`/`take` take and return `std::vector`) over the same
C ABI as the c-ffi demo.

## Build & run

Build the C-API cdylib first, from the ZeroDDS checkout:

```sh
cargo build -p zerodds-c-api --release
```

Then compile against the C++ wrapper headers and link the shared library:

```sh
g++ -std=c++17 hello.cpp \
    -I /path/to/zerodds/crates/cpp/include \
    -I /path/to/zerodds/crates/zerodds-c-api/include \
    -L /path/to/zerodds/target/release -lzerodds \
    -o hello
LD_LIBRARY_PATH=/path/to/zerodds/target/release ./hello
```

## Reference

- Explained on the website: [C++](https://zerodds.org/bindings/cpp/)
- This example in the repository: [zero-dds-snippets/cpp-hello](https://github.com/zero-objects/zero-dds-snippets/tree/main/cpp-hello)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
