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

This is the runnable companion to the code snippet on
**[Python bindings](https://zerodds.de/bindings/python/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
