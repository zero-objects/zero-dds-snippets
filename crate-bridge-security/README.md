# crate-bridge-security

The website's docs/bridge-security.html usage snippet, made runnable.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-bridge-security`.

Output of the run this README was written from:

```
bridge-security: allow_all().check(alice, Write, /topics/trade) = true
```

## Reference

- Explained on the website: [Bridge security](https://zerodds.org/docs/bridge-security.html)
- This example in the repository: [zero-dds-snippets/crate-bridge-security](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-bridge-security)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
