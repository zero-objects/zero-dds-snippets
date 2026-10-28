# crate-security-keyexchange

The website's docs/security-keyexchange.html usage snippet, made runnable.
DDS-Security 1.2 section 9.3.2.3.3: both sides derive the same shared secret
from the peer's public key. `derive_shared_secret` consumes the key pair, so
proving both directions needs the public keys taken first.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-security-keyexchange`.

Output of the run this README was written from:

```
security-keyexchange: both sides derived the same 32-byte secret
```

## Reference

- Explained on the website: [Security key exchange](https://zerodds.org/docs/security-keyexchange.html)
- This example in the repository: [zero-dds-snippets/crate-security-keyexchange](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-security-keyexchange)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
