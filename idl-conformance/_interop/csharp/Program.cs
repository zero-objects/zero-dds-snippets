// XCDR2 wire-level interop golden for the C# PSM (combo::Telemetry).
//
//   ENCODE          -> serialize the canonical sample to raw XCDR2-LE bytes and
//                      write them to the shared golden file.
//   DECODE <file>   -> read raw bytes, reconstruct the sample, assert EVERY
//                      field == the canonical values; exit 0 on match, nonzero
//                      with a diff otherwise.
//
// The bytes ARE the wire: this is the exact XCDR2 a ZeroDDS DataWriter would
// carry. No live pub/sub needed.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using ZeroDDS.Cdr;
using Omg.Types;
using combo;

const string GoldenPathDefault =
    "/path/to/zerodds/zerodds-examples/idl-conformance/_interop/goldens/csharp.bin";

// ---- the canonical sample (identical values across every language PSM) ----
static Telemetry Canonical()
{
    // map<string,long>, encoded key-sorted: "drops" < "packets".
    var counters = new SortedDictionary<string, int>(StringComparer.Ordinal)
    {
        ["drops"] = 3,
        ["packets"] = 100,
    };

    return new Telemetry
    {
        UnitId = 4242,                              // @key long
        Region = "eu-central-1",                    // @key string<32>
        Mode = Mode.MODE_ACTIVE,                    // enum (index 1)
        BatteryCurrent = new CurrentInAmpsType(13.75), // typedef double
        History = new SequenceList<Sample>(new[]    // sequence<Sample>
        {
            new Sample { Seq = 1, Value = 0.5 },
            new Sample { Seq = 2, Value = 1.5 },
            new Sample { Seq = 3, Value = -2.25 },
        }),
        Reading = new Reading                        // union: MODE_ACTIVE -> activeRate
        {
            Discriminator = Mode.MODE_ACTIVE,
            Value = 60.0,
        },
        Counters = counters,                         // map<string,long>
        Calibration = 0.001,                         // @optional double, PRESENT
        Window = new[] { 10, 20, 30, 40 },           // long[4]
    };
}

static int Main(string[] args)
{
    if (args.Length >= 1 && args[0] == "ENCODE")
    {
        var path = args.Length >= 2 ? args[1] : GoldenPathDefault;
        var bytes = TelemetryTypeSupport.Instance.Encode(Canonical(), EndianMode.LittleEndian);
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        File.WriteAllBytes(path, bytes);
        Console.WriteLine($"ENCODE: wrote {bytes.Length} bytes -> {path}");
        return 0;
    }

    if (args.Length >= 1 && args[0] == "DECODE")
    {
        var path = args.Length >= 2 ? args[1] : GoldenPathDefault;
        var bytes = File.ReadAllBytes(path);
        Telemetry got = TelemetryTypeSupport.Instance.Decode(bytes);
        var want = Canonical();

        var fails = new List<string>();
        void Check(string name, bool ok, object? a, object? b)
        {
            Console.WriteLine($"  {(ok ? "OK  " : "FAIL")} {name,-24} want={a} got={b}");
            if (!ok) fails.Add(name);
        }

        Check("unitId", got.UnitId == want.UnitId, want.UnitId, got.UnitId);
        Check("region", got.Region == want.Region, want.Region, got.Region);
        Check("mode", got.Mode == want.Mode, want.Mode, got.Mode);
        Check("batteryCurrent",
            got.BatteryCurrent.Value == want.BatteryCurrent.Value,
            want.BatteryCurrent.Value, got.BatteryCurrent.Value);

        var gh = got.History.ToList();
        var wh = want.History.ToList();
        Check("history.count", gh.Count == wh.Count, wh.Count, gh.Count);
        for (int i = 0; i < Math.Min(gh.Count, wh.Count); i++)
        {
            Check($"history[{i}].seq", gh[i].Seq == wh[i].Seq, wh[i].Seq, gh[i].Seq);
            Check($"history[{i}].value", gh[i].Value == wh[i].Value, wh[i].Value, gh[i].Value);
        }

        Check("reading.disc",
            got.Reading.Discriminator == want.Reading.Discriminator,
            want.Reading.Discriminator, got.Reading.Discriminator);
        Check("reading.activeRate",
            Convert.ToDouble(got.Reading.Value) == Convert.ToDouble(want.Reading.Value),
            want.Reading.Value, got.Reading.Value);

        string ShowMap(IDictionary<string, int> m) =>
            "{" + string.Join(",", m.OrderBy(kv => kv.Key, StringComparer.Ordinal)
                .Select(kv => $"{kv.Key}={kv.Value}")) + "}";
        bool MapEq(IDictionary<string, int> a, IDictionary<string, int> b) =>
            a.Count == b.Count && a.All(kv => b.TryGetValue(kv.Key, out var v) && v == kv.Value);
        Check("counters", MapEq(got.Counters, want.Counters),
            ShowMap(want.Counters), ShowMap(got.Counters));

        Check("calibration",
            got.Calibration.HasValue && got.Calibration.Value == want.Calibration!.Value,
            want.Calibration, got.Calibration);

        Check("window",
            got.Window.SequenceEqual(want.Window),
            string.Join(",", want.Window), string.Join(",", got.Window));

        if (fails.Count == 0)
        {
            Console.WriteLine($"\nDECODE PASS ({bytes.Length} bytes) — every field matches the canonical sample.");
            return 0;
        }
        Console.Error.WriteLine($"\nDECODE FAIL — {fails.Count} field(s) differ: {string.Join(", ", fails)}");
        return 1;
    }

    Console.Error.WriteLine("usage: interop-csharp ENCODE [path] | DECODE [path]");
    return 2;
}

return Main(args);
