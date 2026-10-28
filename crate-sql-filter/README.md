# crate-sql-filter

The website's docs/sql-filter.html usage snippet, made runnable.
DDS 1.4 Annex B: a content filter expression with one positional parameter.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-sql-filter`.

Output of the run this README was written from:

```
sql-filter: "color = %0 AND x > 10" evaluated to true
```

## Reference

- Explained on the website: [SQL filter](https://zerodds.org/docs/sql-filter.html)
- This example in the repository: [zero-dds-snippets/crate-sql-filter](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-sql-filter)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
