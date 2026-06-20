# csharp-quickstart

The minimal pub/sub round-trip for the ZeroDDS C# binding. It creates a participant on domain 0, opens a bytes topic `Chatter`, then writes `"hello"` and takes it back on a same-process reader. Uses the convenience helpers `CreateBytesTopic`, `CreateBytesWriter`/`CreateBytesReader`, `WaitForMatched*`, `Write`, and `Take` — the shortest path to a working DDS exchange in C#.

## Build & run

In this directory:

```sh
dotnet run
```

The `csharp-quickstart.csproj` references the local `../../crates/cs/csharp/ZeroDDS/ZeroDDS.csproj` binding, so no package install is needed. Expected output: `got: hello`.

## Reference

This is the runnable companion to the code snippet on
**[ZeroDDS C# binding](https://zerodds.de/bindings/csharp/)** — see https://zerodds.de/bindings/csharp/.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
