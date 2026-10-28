# crate-amqp-bridge

The website's docs/amqp-bridge.html usage snippet, made runnable.
It claims an AMQP 1.0 `long` survives encode -> decode as AmqpValue::Long.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-amqp-bridge`.

Output of the run this README was written from:

```
amqp-bridge: 2 bytes on the wire, decoded Long(42)
```

## Reference

- Explained on the website: [AMQP bridge](https://zerodds.org/docs/amqp-bridge.html)
- This example in the repository: [zero-dds-snippets/crate-amqp-bridge](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-amqp-bridge)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
