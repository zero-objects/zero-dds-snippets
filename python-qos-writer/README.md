# python-qos-writer

Shows how to configure QoS on a Python writer. It builds a `DataWriterQos`, sets `Reliable` reliability, `TransientLocal` durability, and `KeepLast`/depth-1 history — the classic "state topic with a late-joiner cache" recipe — then creates the writer via `create_bytes_writer_with_qos`. Proves the QoS policies are applied by reading them back (`durability_kind()`, `reliability_kind()`, `history_kind()`/`history_depth()`) and confirming the writer was constructed.

## Build & run

Build the Python extension once from the ZeroDDS checkout:

```sh
cd crates/py && maturin develop
```

Then run the demo (with the built `libzerodds` on the loader path):

```sh
python qos_writer.py
```

## Reference

This is the runnable companion to the code snippet on
**[Python bindings](https://zerodds.de/bindings/python/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
