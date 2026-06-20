# hello-publisher

The minimal quickstart publisher, matching the README/user-guide exactly. It grabs the `DomainParticipantFactory`, creates an **offline** participant (`create_participant_offline`, no network runtime), a `RawBytes` topic `"Greetings"`, a writer, writes `b"Hello, DDS!"`, and prints a confirmation. The smallest possible "write one sample" program.

## Build & run

In the demo dir:

```sh
cargo run
```

No features to enable. Depends only on `zerodds-dcps` (`1.0.0-rc.3`, from crates.io). Being offline, it needs no peer — it just constructs the entities and writes.

## Reference

This is the runnable companion to the code snippet on
**[User guide](https://zerodds.de/docs/user-guide/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
