# crate-amqp-endpoint

The website's docs/amqp-endpoint.html usage snippet, made runnable.
A sender link grants credit, delivers, and settles; the settlement count
must return to zero.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-amqp-endpoint`.

Output of the run this README was written from:

```
amqp-endpoint: link settled, pending_settlements = 0
```

## Reference

- Explained on the website: [AMQP endpoint](https://zerodds.org/docs/amqp-endpoint.html)
- This example in the repository: [zero-dds-snippets/crate-amqp-endpoint](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-amqp-endpoint)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
