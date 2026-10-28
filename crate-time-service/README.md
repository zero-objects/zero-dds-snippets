# crate-time-service

The website's docs/time-service.html usage snippet, made runnable.
OMG Time Service 1.1: a Uto one millisecond later is not less than now.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-time-service`.

Output of the run this README was written from:

```
time-service: now vs now+1ms compares LessThan
```

## Reference

- Explained on the website: [Time service](https://zerodds.org/docs/time-service.html)
- This example in the repository: [zero-dds-snippets/crate-time-service](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-time-service)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
