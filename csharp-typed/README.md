# csharp-typed

Demonstrates strongly-typed topics driven by IDL codegen. `Temperature.cs` and `Temperature.TypeSupport.cs` are the verbatim `zerodds-idlc generate --lang csharp temperature.idl` output (the type, its codec over `ZeroDds.Wire`, and `TemperatureTypeSupport`). `Program.cs` runs the website snippet — `CreateTypedTopic<Temperature>`, `CreateTypedWriter<Temperature>`, and `writer.Write(new Temperature { Celsius = 23, Sensor_id = "A7" })` — proving generated TypeSupport plugs straight into the binding. The generator keeps the IDL member name `sensor_id` as `Sensor_id`.

## Build & run

In this directory:

```sh
dotnet run
```

The `csharp-typed.csproj` references the local `../../crates/cs/csharp/ZeroDDS/ZeroDDS.csproj` binding, so no package install is needed. It compiles `Program.cs` and the two generated files. Expected output reports the written `Temperature` on topic `Temp` with type `sensor_msgs::msg::Temperature`.

## Reference

- Explained on the website: [C# / .NET](https://zerodds.org/bindings/csharp/)
- This example in the repository: [zero-dds-snippets/csharp-typed](https://github.com/zero-objects/zero-dds-snippets/tree/main/csharp-typed)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
