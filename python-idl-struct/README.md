# python-idl-struct

Shows typed (IDL-style) topics in Python. A `@dataclass` is decorated with `@idl_struct(typename="sensor_msgs::msg::Temperature")` and uses the `Int32`/`String`/`Bytes` field types from `zerodds.idl`, then published over an `IdlTopic`. Proves structured CDR data — not raw bytes — round-trips: a `Temperature(celsius=23, sensor_id="A7")` sample is written and read back as a fully reconstructed dataclass instance.

## Build & run

Build the Python extension once from the ZeroDDS checkout:

```sh
cd crates/py && maturin develop
```

Then run the demo (with the built `libzerodds` on the loader path):

```sh
python temp.py
```

## Reference

This is the runnable companion to the code snippet on
**[Python bindings](https://zerodds.de/bindings/python/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
