# csharp-typed

Demonstrates strongly-typed topics driven by IDL codegen. `Temperature.cs` is the verbatim `idlc csharp temperature.idl` output (record + `TemperatureTypeSupport` with XCDR2 appendable encode/decode). `Program.cs` runs the exact website snippet — `CreateTypedTopic<Temperature>`, `CreateTypedWriter<Temperature>`, and `writer.Write(new Temperature { Celsius = 23, SensorId = "A7" })` — proving generated TypeSupport plugs straight into the binding. `OmgTypesShim.cs` supplies just the `Omg.Types.ITopicType<T>` marker the generated code needs.

## Build & run

In this directory:

```sh
dotnet run
```

The `csharp-typed.csproj` references the local `../../crates/cs/csharp/ZeroDDS/ZeroDDS.csproj` binding, so no package install is needed. It compiles `Program.cs`, the generated `Temperature.cs`, and `OmgTypesShim.cs`. Expected output reports the written `Temperature` on topic `Temp` with type `sensor_msgs::msg::Temperature`.

## Reference

This is the runnable companion to the code snippet on
**[ZeroDDS C# binding](https://zerodds.de/bindings/csharp/)** — see https://zerodds.de/bindings/csharp/.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
