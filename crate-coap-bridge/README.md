# crate-coap-bridge

The website's docs/coap-bridge.html usage snippet, made runnable.
RFC 7252 header round-trip: a Confirmable GET keeps its code across the wire.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-coap-bridge`.

Output of the run this README was written from:

```
coap-bridge: 4 bytes on the wire, decoded code CoapCode { class: 0, detail: 1 }
```

## Reference

- Explained on the website: [CoAP bridge](https://zerodds.org/docs/coap-bridge.html)
- This example in the repository: [zero-dds-snippets/crate-coap-bridge](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-coap-bridge)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
