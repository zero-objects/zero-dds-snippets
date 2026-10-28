# crate-transport-udp

The website's docs/transport-udp.html usage snippet, made runnable.

The snippet names `dest` and `datagram` without defining them, and binds the
fixed ports 7410 / 7400. This binds ephemeral ports so it does not collide
with a running participant, then actually sends a datagram to itself and
reads it back — the send/recv pair the snippet shows.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-transport`, `zerodds-transport-udp`.

Output of the run this README was written from:

```
transport-udp: bound Locator { kind: UdpV4, port: 61069, address: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 1] }
transport-udp: 4 bytes back from Locator { kind: UdpV4, port: 62119, address: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 1] }
transport-udp: joined 239.255.0.1 at Locator { kind: UdpV4, port: 49337, address: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 239, 255, 0, 1] }
```

## Reference

- Explained on the website: [UDP transport](https://zerodds.org/docs/transport-udp.html)
- This example in the repository: [zero-dds-snippets/crate-transport-udp](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-transport-udp)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
