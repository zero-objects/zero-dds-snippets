# qos-policies-full-writer

The runnable companion to **[QoS policies](https://github.com/zero-objects/zero-dds/blob/main/documentation/03-configuration/qos-policies.md)**
— "Setting QoS in code". Builds the exact `UserWriterConfig` from the doc
(`TransientLocal` durability, `Exclusive` ownership + strength 100,
`Deadline`/`Lifespan`/`Liveliness` policies, `sensor.*` partition) and
registers it against a real `DcpsRuntime`, proving the combination is
accepted at registration time — not just that the struct literal type-checks.

## Build & run

Needs a checked-out [ZeroDDS source tree](https://github.com/zero-objects/zero-dds)
next to this repo (path deps into `crates/dcps`, `crates/qos`, `crates/rtps`,
`crates/types`).

```sh
cargo run
```

Expected output:

```text
qos-policies-full-writer: registered EntityId(...)
```

## Reference

This is the runnable companion to the code snippet on
**[documentation/03-configuration/qos-policies.md](https://github.com/zero-objects/zero-dds/blob/main/documentation/03-configuration/qos-policies.md)**.

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
