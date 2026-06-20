# c-ffi-hello

A minimal C program (`hello.c`) that drives ZeroDDS through the raw C FFI in
`zerodds.h`. It creates a runtime, makes a reliable `RawBytes` writer and reader
on topic `Chatter`, waits for them to match, publishes `"hello"`, and drains the
reader with the fixed-buffer convenience call `zerodds_reader_take_into`. It
proves the plain-C ABI (`zerodds_runtime_create` / `zerodds_writer_write` /
`zerodds_reader_take_into`) is usable end-to-end without any C++ wrapper.

## Build & run

Build the C-API cdylib first, from the ZeroDDS checkout:

```sh
cargo build -p zerodds-c-api --release
```

Then compile and link against the header and the shared library:

```sh
gcc -std=c11 hello.c \
    -I /path/to/zerodds/crates/zerodds-c-api/include \
    -L /path/to/zerodds/target/release -lzerodds \
    -o hello
LD_LIBRARY_PATH=/path/to/zerodds/target/release ./hello
```

## Reference

This is the runnable companion to the code snippet on
**[C FFI bindings](https://zerodds.de/bindings/c-ffi/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
