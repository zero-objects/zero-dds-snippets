# crate-time-service

The website's docs/time-service.html usage snippet, made runnable.
OMG Time Service 1.1: a Uto one millisecond later is not less than now.

## Run it

```sh
cargo run
```

Builds `zerodds-time-service` from the source of zero-dds (`main`) until
1.0.0-rc.8 is on crates.io: `Uto::new` returns a `Result` there (BAD_PARAM
above 48 bits), in the published 1.0.0-rc.7.2 it returns the `Uto`.

Output of the run this README was written from:

```
time-service: now vs now+1ms compares LessThan
```

## Reference

- Explained on the website: [Time service](https://zerodds.org/docs/time-service.html)
- This example in the repository: [zero-dds-snippets/crate-time-service](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-time-service)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
