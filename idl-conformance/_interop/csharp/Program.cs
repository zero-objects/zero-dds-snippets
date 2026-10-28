// idl-conformance / _interop / csharp — the combo::Telemetry column.
//
//   dotnet interop-csharp.dll ENCODE <out.bin>   write the canonical sample as raw XCDR2-LE
//   dotnet interop-csharp.dll DECODE <in.bin>    decode, assert every field == canonical
//
// combo.cs and combo.TypeSupport.cs are `zerodds-idlc generate --lang csharp`
// output of ../combo.idl over runtime/csharp; values are CANONICAL.md.

using System;
using System.IO;
using System.Linq;
using ZeroDDS.Cdr;
using combo;

Telemetry Canonical() => new Telemetry
{
    UnitId = 4242,                                   // @key long
    Region = "eu-central-1",                         // @key string<32>
    Mode = Mode.MODE_ACTIVE,                         // enum, index 1
    BatteryCurrent = 13.75,                          // typedef double
    History = new[]                                  // sequence<Sample>
    {
        new Sample { Seq = 1, Value = 0.5 },
        new Sample { Seq = 2, Value = 1.5 },
        new Sample { Seq = 3, Value = -2.25 },
    },
    Reading = new Reading { _d = Mode.MODE_ACTIVE, ActiveRate = 60.0 },
    Counters = new[]                                 // map<string,long>, key-sorted
    {
        new ZdEntry_string_long { Key = "drops", Value = 3 },
        new ZdEntry_string_long { Key = "packets", Value = 100 },
    },
    Calibration = 0.001,                             // @optional double, present
    Window = new[] { 10, 20, 30, 40 },               // long[4]
};

if (args.Length != 2 || (args[0] != "ENCODE" && args[0] != "DECODE"))
{
    Console.Error.WriteLine("usage: interop-csharp ENCODE <out.bin> | DECODE <in.bin>");
    return 2;
}

if (args[0] == "ENCODE")
{
    byte[] bytes = TelemetryTypeSupport.Instance.Encode(Canonical(), EndianMode.LittleEndian);
    File.WriteAllBytes(args[1], bytes);
    Console.WriteLine($"ENCODE OK: {bytes.Length} bytes -> {args[1]}");
    return 0;
}

Telemetry g = TelemetryTypeSupport.Instance.Decode(File.ReadAllBytes(args[1]), EndianMode.LittleEndian);
Telemetry w = Canonical();
int failures = 0;
void Check(bool ok, string what)
{
    if (ok) return;
    failures++;
    Console.Error.WriteLine($"  [FAIL] {what}");
}

Check(g.UnitId == w.UnitId, "unitId");
Check(g.Region == w.Region, "region");
Check(g.Mode == w.Mode, "mode");
Check(g.BatteryCurrent == w.BatteryCurrent, "batteryCurrent");
Check(g.History.Length == w.History.Length
      && g.History.Zip(w.History).All(p => p.First.Seq == p.Second.Seq && p.First.Value == p.Second.Value), "history");
Check(g.Reading._d == w.Reading._d && g.Reading.ActiveRate == w.Reading.ActiveRate, "reading");
Check(g.Counters.Length == w.Counters.Length
      && g.Counters.Zip(w.Counters).All(p => p.First.Key == p.Second.Key && p.First.Value == p.Second.Value), "counters");
Check(g.Calibration == w.Calibration, "calibration");
Check(g.Window.SequenceEqual(w.Window), "window");

if (failures != 0)
{
    Console.WriteLine($"DECODE FAILED: {failures} mismatch(es)");
    return 1;
}
Console.WriteLine("DECODE OK: every field equals canonical");
return 0;
