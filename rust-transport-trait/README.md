# rust-transport-trait

Implements the `zerodds_transport::Transport` SPI with a `NullTransport`, proving the trait snippet on the docs matches the live API. The trait shows exactly three methods — `send(&self, dest, data)`, `recv(&self)`, and `local_locator(&self)` — and the demo implements all three with those exact signatures, then exercises each one and prints the result.

## Build & run

In the demo dir:

```sh
cargo run
```

No features to enable. Depends only on `zerodds-transport` via a path dep into the local zero-dds checkout (until the API ships on crates.io).

## Reference

This is the runnable companion to the code snippet on
**[Transport reference](https://zerodds.de/topics/transport-reference/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
