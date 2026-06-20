# python-aio

Shows the `asyncio`-friendly ZeroDDS API from `zerodds.aio`. It wraps a bytes writer/reader in `AsyncBytesWriter`/`AsyncBytesReader` and `await`s `wait_for_matched_subscription`, `wait_for_matched_publication`, `write`, and `wait_for_data` — so DDS discovery and I/O integrate cleanly into an `asyncio` event loop instead of blocking. Proves a full async round trip: one participant publishes `b"async hello"` and reads it back via `take()`.

## Build & run

Build the Python extension once from the ZeroDDS checkout:

```sh
cd crates/py && maturin develop
```

Then run the demo (with the built `libzerodds` on the loader path):

```sh
python async_chatter.py
```

## Reference

This is the runnable companion to the code snippet on
**[Python bindings](https://zerodds.de/bindings/python/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
