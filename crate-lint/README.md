# crate-lint



## Run it

```sh
ZERODDS_SRC=/path/to/zero-dds ./run.sh
```

It needs no dependencies of its own: the two fixture crates are input to `zerodds-lint`, not code that links against ZeroDDS. `run.sh` builds `zerodds-lint` in the checkout (`cargo build -p zerodds-lint --release`) and runs it.

Output of the run this README was written from (paths shortened to this
repository):

```
== without the pragmas ==
crate-lint/no-pragmas/src/lib.rs:10:30 [error] dds_no_dyn_in_safe: `dyn Trait` in a SAFE crate; use concrete generics, or mark the file with `zerodds-lint: allow no_dyn_in_safe`
crate-lint/no-pragmas/src/lib.rs:35:8 [error] dds_bounded_recursion: function `parse_module` is directly recursive; add a doc comment `/// zerodds-lint: recursion-depth N` with the expected depth
zerodds-lint: scanned 1 crates, 1 files; 2 errors, 0 warnings

== with the pragmas ==
zerodds-lint: scanned 1 crates, 1 files; 0 errors, 0 warnings

lint: 2 errors without the pragmas, 0 with them
```

## Reference

- Explained on the website: [zerodds-lint](https://zerodds.org/docs/lint.html)
- This example in the repository: [zero-dds-snippets/crate-lint](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-lint)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
