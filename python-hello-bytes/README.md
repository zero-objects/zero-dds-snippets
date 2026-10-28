# python-hello-bytes

The minimal ZeroDDS "hello world" in Python: create a `DomainParticipantFactory`, a participant, a bytes topic, then a publisher/writer and subscriber/reader pair. It waits for discovery to match (`wait_for_matched_subscription`/`wait_for_matched_publication`), writes a `b"hello"` payload, and reads it back with `take()`. Proves the synchronous bytes API end to end in a handful of lines.

## Build & run

Build the Python extension once from the ZeroDDS checkout:

```sh
cd crates/py && maturin develop
```

Then run the demo (with the built `libzerodds` on the loader path):

```sh
python hello.py
```

## Reference

- Explained on the website: [Python](https://zerodds.org/bindings/python/)
- This example in the repository: [zero-dds-snippets/python-hello-bytes](https://github.com/zero-objects/zero-dds-snippets/tree/main/python-hello-bytes)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
