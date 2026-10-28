# csharp-quickstart

The minimal pub/sub round-trip for the ZeroDDS C# binding. It creates a participant on domain 0, opens a bytes topic `Chatter`, then writes `"hello"` and takes it back on a same-process reader. Uses the convenience helpers `CreateBytesTopic`, `CreateBytesWriter`/`CreateBytesReader`, `WaitForMatched*`, `Write`, and `Take` — the shortest path to a working DDS exchange in C#.

## Build & run

In this directory:

```sh
dotnet run
```

The `csharp-quickstart.csproj` references the local `../../crates/cs/csharp/ZeroDDS/ZeroDDS.csproj` binding, so no package install is needed. Expected output: `got: hello`.

## Reference

- Explained on the website: [C# / .NET](https://zerodds.org/bindings/csharp/)
- This example in the repository: [zero-dds-snippets/csharp-quickstart](https://github.com/zero-objects/zero-dds-snippets/tree/main/csharp-quickstart)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
