# crate-transport-tcp

The website's docs/transport-tcp.html usage snippet, made runnable.

The snippet is two lines: bind port 7400, then `accept_one()`. Both are
kept, with two changes a run forces:

 1. Port 7400 is the well-known DDS discovery port and is often already
    taken on a test host, so this binds an ephemeral port and prints it.
 2. `accept_one()` does not accept a bare TCP connection — it performs the
    16-byte ZeroDDS handshake and then reads frames until the peer closes.
    A plain `TcpStream::connect` leaves it waiting and it fails with
    `Handshake(Io(WouldBlock))` after its 2 s budget. The peer here is a
    second `TcpTransport` sending through the `Transport` trait, which is
    what speaks that handshake.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-transport`, `zerodds-transport-tcp`.

Output of the run this README was written from:

```
transport-tcp: listening on 127.0.0.1:55349
transport-tcp: accepted one connection, 13 bytes from Tcpv4
transport-tcp: payload roundtripped
```

## Reference

- Explained on the website: [TCP transport](https://zerodds.org/docs/transport-tcp.html)
- This example in the repository: [zero-dds-snippets/crate-transport-tcp](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-transport-tcp)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
