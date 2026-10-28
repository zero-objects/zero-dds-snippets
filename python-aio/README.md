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

- Explained on the website: [Python](https://zerodds.org/bindings/python/)
- This example in the repository: [zero-dds-snippets/python-aio](https://github.com/zero-objects/zero-dds-snippets/tree/main/python-aio)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
