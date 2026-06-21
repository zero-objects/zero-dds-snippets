// IDL conformance — combined-feature round-trip over a REAL DCPS loopback.
//
// Builds one fully-populated `confcombo::Telemetry` sample, publishes it with a
// typed DataWriter, takes it back on a same-participant DataReader, and asserts
// every field survived the XCDR2 wire encode/decode.
//
// Features exercised together (see conformance_combo.idl):
//   enum, bounded string<32>, unbounded sequence<long>, bounded sequence<long,8>,
//   sequence<string>, @optional double (present + absent), two @key members.

using System;
using System.Collections.Generic;
using System.Linq;
using ZeroDDS;
using ZeroDDS.Cdr;
using ZeroDDS.Core;
using Omg.Types;
using confcombo;

static SequenceList<T> Seq<T>(params T[] items) => new(items);

// ---- the sample we publish (every supported feature populated) ----
var sent = new Telemetry
{
    UnitId = 42,                                  // @key long
    Region = "eu-central-1",                      // @key string<32>
    Mode = Mode.MODE_ACTIVE,                       // enum
    BatteryVolts = 12.6,                           // double
    Samples = Seq(1, 2, 3, 5, 8, 13),              // unbounded sequence<long>
    RecentCodes = Seq(10, 20, 30, 40),             // bounded sequence<long,8>
    Tags = Seq("alpha", "beta", "gamma"),          // sequence<string>
    Calibration = 0.0078125,                       // @optional double (present)
};

var factory = DomainParticipantFactory.Instance;
using var participant = factory.CreateParticipant(0);
var publisher = participant.CreatePublisher();
var subscriber = participant.CreateSubscriber();

var topic = participant.CreateTypedTopic<Telemetry>("ConfCombo");
using var writer = publisher.CreateTypedWriter<Telemetry>(topic);
using var reader = subscriber.CreateTypedReader<Telemetry>(topic);

writer.WaitForMatched(1, Duration.FromSeconds(5));
reader.WaitForMatched(1, Duration.FromSeconds(5));

writer.Write(sent);
Console.WriteLine($"published Telemetry on topic '{topic.Name}' (type {topic.TypeName})");

if (!reader.WaitForData(TimeSpan.FromSeconds(5)))
{
    Console.Error.WriteLine("FAIL: no data received within 5s");
    Environment.Exit(1);
}

var samples = reader.Take();
var valid = samples.Where(s => s.Info.ValidData).Select(s => s.Data).ToList();
if (valid.Count == 0)
{
    Console.Error.WriteLine("FAIL: took 0 valid samples");
    Environment.Exit(1);
}

var got = valid[0];
Console.WriteLine("recovered sample from the wire — asserting equality field-by-field");

var failures = new List<string>();
void Check(string name, bool ok, object? a, object? b)
{
    Console.WriteLine($"  {(ok ? "OK  " : "FAIL")} {name,-14} sent={a} got={b}");
    if (!ok) failures.Add(name);
}

static bool SeqEq<T>(IEnumerable<T>? a, IEnumerable<T>? b) =>
    (a ?? Enumerable.Empty<T>()).SequenceEqual(b ?? Enumerable.Empty<T>());

Check("unitId", got.UnitId == sent.UnitId, sent.UnitId, got.UnitId);
Check("region", got.Region == sent.Region, sent.Region, got.Region);
Check("mode", got.Mode == sent.Mode, sent.Mode, got.Mode);
Check("batteryVolts", got.BatteryVolts == sent.BatteryVolts, sent.BatteryVolts, got.BatteryVolts);
Check("samples", SeqEq(got.Samples, sent.Samples),
    string.Join(",", sent.Samples), string.Join(",", got.Samples));
Check("recentCodes", SeqEq(got.RecentCodes, sent.RecentCodes),
    string.Join(",", sent.RecentCodes), string.Join(",", got.RecentCodes));
Check("tags", SeqEq(got.Tags, sent.Tags),
    string.Join(",", sent.Tags), string.Join(",", got.Tags));
Check("calibration", got.Calibration == sent.Calibration, sent.Calibration, got.Calibration);

// Exercise @optional ABSENT too: a second sample with a null optional must
// round-trip as null (not 0.0).
var sent2 = sent with { UnitId = 43, Calibration = null };
writer.Write(sent2);
reader.WaitForData(TimeSpan.FromSeconds(5));
var got2 = reader.Take().Where(s => s.Info.ValidData).Select(s => s.Data)
    .FirstOrDefault(s => s.UnitId == 43);
if (got2 is null)
{
    failures.Add("optional-absent (sample not received)");
}
else
{
    Check("calibration=null", got2.Calibration is null, "null", got2.Calibration);
}

if (failures.Count == 0)
{
    Console.WriteLine("\nROUNDTRIP PASS — all supported fields survived the DCPS wire.");
}
else
{
    Console.Error.WriteLine($"\nROUNDTRIP FAIL — {failures.Count} field(s) differ: " +
        string.Join(", ", failures));
    Environment.Exit(1);
}
