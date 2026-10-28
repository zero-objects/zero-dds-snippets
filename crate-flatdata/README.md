# crate-flatdata

The website's docs/flatdata.html usage snippet, made runnable.
A POD struct written into a shared-memory slot and read straight back out,
with no serialization step in between.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-flatdata`.

Output of the run this README was written from:

```
flatdata: 24 bytes per slot, roundtripped Pose { x: 1.0, y: 2.0, z: 3.0 }
```

## Reference

- Explained on the website: [FlatData](https://zerodds.org/docs/flatdata.html)
- This example in the repository: [zero-dds-snippets/crate-flatdata](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-flatdata)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
