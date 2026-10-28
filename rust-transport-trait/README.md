# rust-transport-trait

Implements the `zerodds_transport::Transport` SPI with a `NullTransport`, proving the trait snippet on the docs matches the live API. The trait shows exactly three methods — `send(&self, dest, data)`, `recv(&self)`, and `local_locator(&self)` — and the demo implements all three with those exact signatures, then exercises each one and prints the result.

## Build & run

In the demo dir:

```sh
cargo run
```

No features to enable. Depends only on `zerodds-transport` via a path dep into the local zero-dds checkout (until the API ships on crates.io).

## Reference

- Explained on the website: [Transport Reference](https://zerodds.org/topics/transport-reference/)
- This example in the repository: [zero-dds-snippets/rust-transport-trait](https://github.com/zero-objects/zero-dds-snippets/tree/main/rust-transport-trait)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
