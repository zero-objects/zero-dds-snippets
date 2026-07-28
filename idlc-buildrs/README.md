# idlc-buildrs

The runnable companion to **[`zerodds-idlc` Handbook](https://github.com/zero-objects/zero-dds/blob/main/documentation/04-idl/idlc-handbook.md)**
— section "6 build.rs integration (Rust)", covering both fences on that page:
the `build.rs` that shells out to the real `zerodds-idlc` CLI, and the
`lib.rs` `include!` that pulls the generated module in.

`idl/Robot.idl` defines an `@appendable`, `@key`-ed `Robot::Pose` struct.
`build.rs` runs `zerodds-idlc generate idl/Robot.idl --rust -o $OUT_DIR`
exactly as the doc shows, then `src/lib.rs` `include!`s
`$OUT_DIR/Robot.rs`. `src/bin/idlc-buildrs-demo` (`src/main.rs`) constructs
a `Robot::Pose`, round-trips it through `DdsType::encode`/`decode`, and
prints the result; `cargo test` additionally proves the roundtrip via a
unit test on the library target.

## A doc gap this companion found (and the doc got fixed for)

The doc's `build.rs` fence, as originally published, called
`zerodds-idlc generate --rust -o $OUT_DIR` and then just `include!`d the
output directly. That does not compile: the full `--rust` backend's
generated file always carries file-level `#![allow(...)]` inner
attributes (it is designed to stand alone as its own module file), and
`include!` splices those tokens into the *including* file — where an
inner attribute is only legal if it is the very first thing in that file,
which is not the case once anything (even a doc comment) precedes the
`include!` line. Both the doc and this companion's `build.rs` now strip
the `#![...]` lines from the generated output and re-apply the same
allows as outer attributes on the `include!` call site (`lib.rs`'s
`#[allow(...)] mod robot { include!(...); }` wrapper). `clippy::useless_conversion`
is additionally allowed there — a pre-existing, unrelated `zerodds-idl-rust`
codegen quirk (a needless `.into()` in the bounded-string-length error
arm), not something a documentation companion should silently paper over
without saying so, and not fixed here since it touches the shared codegen
backend used by all consumers.

## Build & run

Needs:

- A checked-out [ZeroDDS source tree](https://github.com/zero-objects/zero-dds)
  next to this repo (path deps into `crates/cdr`, `crates/dcps`,
  `crates/types`, `crates/sql-filter`).
- The `zerodds-idlc` binary on `PATH` — build it from that checkout
  (`cargo build -p zerodds-idlc`, then add `target/debug` to `PATH`) or
  install it per the handbook's [Installation](https://github.com/zero-objects/zero-dds/blob/main/documentation/04-idl/idlc-handbook.md#2-installation)
  section.

```sh
cargo test   # roundtrip unit test on the lib target
cargo run --bin idlc-buildrs-demo
```

Expected output:

```text
idlc-buildrs-demo: 40 bytes on the wire, roundtripped Pose { robot_id: "R2D2", x: 1.0, y: 2.0, theta: 0.5 }
```

## Reference

This is the runnable companion to the code snippets on
**[documentation/04-idl/idlc-handbook.md](https://github.com/zero-objects/zero-dds/blob/main/documentation/04-idl/idlc-handbook.md)**.

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
