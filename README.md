# ZeroDDS — Snippets & Examples

Runnable companions to the code snippets on the [ZeroDDS website](https://zerodds.org)
and README. Each website snippet has its own minimal project here that compiles
and runs the exact code shown in the docs, so the published examples can never
silently drift from the API.

## Layout

One directory per snippet, grouped by language/binding:

| Prefix | Binding |
|--------|---------|
| `bindings-rust-*`, `rust-*`, `hello-publisher`, `async-pubsub`, `qos-cookbook`, `pubsub-roundtrip` | Rust (`zerodds-dcps`) |
| `python-*` | Python (`zerodds`, maturin) |
| `java-*` | Java (`org.omg.dds.*`, OMG DDS-Java-PSM) |
| `csharp-*` | C# / .NET (`ZeroDDS`) |
| `typescript-node-*` | TypeScript (Node, `@zerodds/node`) |
| `typescript-wasm-*` | TypeScript (WASM, `@zerodds/wasm`) |
| `cpp-*`, `c-ffi-*` | C++ / C-API |

## Building

The original Rust examples (`hello-publisher`, `pubsub-roundtrip`) build straight
against the published `zerodds-dcps` on crates.io.

> **Note:** Most other demos use path dependencies (`path = "../../crates/..."`,
> a local `dist/`, the maturin-built extension) into a checked-out
> [ZeroDDS source tree](https://github.com/zero-objects/zero-dds), because some
> APIs they exercise are newer than the latest crates.io / npm release. Place
> this repository next to a `zero-dds` checkout, or adjust the dependency paths.
> Once the release containing these APIs ships, the demos switch to the
> published packages.

Per-language quickstart:

- **Rust:** `cd <demo> && cargo run`
- **Python:** `maturin develop` in the ZeroDDS `crates/py`, then `python <demo>/*.py`
- **Java:** `mvn -q install` in `crates/java-omgdds/java`, then `javac`/`java` against the jar
- **C# / .NET:** `dotnet run` in the demo
- **TypeScript:** `npm install && npm run build` in the binding, then `npx tsc` / `tsx` the demo
- **C++ / C:** build the C-API cdylib, then `g++`/`gcc -I<includes> -lzerodds`

## License

Apache-2.0, same as ZeroDDS.
