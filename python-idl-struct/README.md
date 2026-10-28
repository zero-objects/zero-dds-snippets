# python-idl-struct

Shows typed (IDL) topics in Python. `temperature.py` is the `zerodds-idlc generate --lang python temperature.idl` output: a dataclass for `sensor_msgs::msg::Temperature`, its codec over `zerodds_wire`, and `ZD_TOPIC_DESCRIPTORS`. `temp.py` publishes it over an `IdlTopic`. Proves structured CDR data — not raw bytes — round-trips: a `Temperature(celsius=23, sensor_id="A7", raw_blob=[])` sample is written and read back as a fully reconstructed dataclass instance.

## Build & run

Build the Python extension once from the ZeroDDS checkout (it needs the
`zerodds-wire` runtime from `runtime/python` next to it):

```sh
pip install runtime/python
cd crates/py && maturin develop
```

To regenerate `temperature.py` after editing the IDL:

```sh
cargo run -p zerodds-idlc -- generate --lang python temperature.idl --out .
```

Then run the demo (with the built `libzerodds` on the loader path):

```sh
python temp.py
```

## Reference

- Explained on the website: [Python](https://zerodds.org/bindings/python/)
- This example in the repository: [zero-dds-snippets/python-idl-struct](https://github.com/zero-objects/zero-dds-snippets/tree/main/python-idl-struct)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
