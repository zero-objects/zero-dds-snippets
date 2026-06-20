# csharp-async

Demonstrates the async/await surface of the ZeroDDS C# binding. After matching a `DataWriter<byte[]>` and `DataReader<byte[]>` on topic `AsyncChatter`, it runs the verbatim website snippet: `await writer.WriteAsync(...)`, `await reader.WaitForDataAsync(...)`, and `await foreach (var sample in reader.TakeAsync(ct))`. It proves the reader's async stream and cancellation-token plumbing deliver a same-process loopback sample without blocking.

## Build & run

In this directory:

```sh
dotnet run
```

The `csharp-async.csproj` references the local `../../crates/cs/csharp/ZeroDDS/ZeroDDS.csproj` binding, so no package install is needed. Expected final line: `async demo OK`.

## Reference

This is the runnable companion to the code snippet on
**[ZeroDDS C# binding](https://zerodds.de/bindings/csharp/)** — see https://zerodds.de/bindings/csharp/.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
