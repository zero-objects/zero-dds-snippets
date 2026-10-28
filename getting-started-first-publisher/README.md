# getting-started-first-publisher

The runnable companion to **[First publisher and subscriber](https://github.com/zero-objects/zero-dds/blob/main/documentation/01-getting-started/first-publisher.md)**
— the getting-started chapter that wires a publisher and subscriber together
using the low-level Rust API directly: `DcpsRuntime::start`,
`register_user_writer` / `register_user_reader`, `write_user_sample`, and the
`UserSample::{Alive, Lifecycle}` receive loop. Two binaries, `pub` and `sub`,
verbatim from the chapter's `src/bin/pub.rs` / `src/bin/sub.rs` — if either
stops compiling, the doc's API drifted from the codebase.

## Build & run

Needs a checked-out [ZeroDDS source tree](https://github.com/zero-objects/zero-dds)
next to this repo (path deps into `crates/dcps`, `crates/qos`, `crates/rtps`,
`crates/types` — this chapter's API is ahead of the last crates.io release).

```sh
cargo build
```

In two terminals:

```sh
# Terminal 1
cargo run --bin sub

# Terminal 2 (start ~1 second later so the subscriber is listening)
cargo run --bin pub
```

Expected output in terminal 1:

```text
sub: got hello 0
sub: got hello 1
sub: got hello 2
...
```

Domain 1, `HelloTopic`/`RawBytes`, reliable + volatile — matches the chapter
exactly. `sub` reads for 15 s then shuts down; `pub` sends 10 samples 500 ms
apart, so run `sub` first.

## Reference

- Explained in the ZeroDDS tree: [documentation/01-getting-started/first-publisher.md](https://github.com/zero-objects/zero-dds/blob/main/documentation/01-getting-started/first-publisher.md)
- This example in the repository: [zero-dds-snippets/getting-started-first-publisher](https://github.com/zero-objects/zero-dds-snippets/tree/main/getting-started-first-publisher)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
