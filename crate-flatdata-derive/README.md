# crate-flatdata-derive

The website's docs/flatdata-derive.html usage snippet, made runnable.
The derive writes the `FlatStruct` impl the hand-written example spells out;
the page states the TYPE_HASH is the first 16 bytes of a SHA-256 over the
structural signature, so the demo prints it rather than asserting a constant
the page does not name.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-flatdata`, `zerodds-flatdata-derive`.

Output of the run this README was written from:

```
flatdata-derive: Pose WIRE_SIZE = 24, TYPE_HASH = 46a24a612f7ea2a3ade887ca4cc74a34
```

## Reference

- Explained on the website: [FlatData derive](https://zerodds.org/docs/flatdata-derive.html)
- This example in the repository: [zero-dds-snippets/crate-flatdata-derive](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-flatdata-derive)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
