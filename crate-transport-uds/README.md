# crate-transport-uds

The website's docs/transport-uds.html usage snippet, made runnable.
The snippet names `local_id`, `dest` and `datagram` without defining them;
this supplies a GUID prefix, sends a datagram to the socket's own locator
and reads it back.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-transport`, `zerodds-transport-uds`.

Output of the run this README was written from:

```
transport-uds: bound Locator { kind: Uds, port: 0, address: [90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90] }
transport-uds: 4 bytes back over the unix socket
```

## Reference

- Explained on the website: [UDS transport](https://zerodds.org/docs/transport-uds.html)
- This example in the repository: [zero-dds-snippets/crate-transport-uds](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-transport-uds)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
