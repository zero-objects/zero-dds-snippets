// Combined-feature acceptance test of fresh generated types over native DCPS.
// Optional absence is mandatory, never a warning or a skipped assertion.
using System;
using System.Linq;
using ZeroDDS;
using ZeroDDS.Core;
using confcombo;

static void Require(bool condition, string message)
{
    if (!condition) throw new InvalidOperationException(message);
}

static Telemetry MakeSample(int unitId, double? calibration) => new()
{
    UnitId = unitId,
    Region = "eu-central-1",
    Mode = Mode.MODE_ACTIVE,
    BatteryVolts = 12.6,
    BatteryCurrent = 3.75,
    Location = new Position { X = 1.0, Y = 2.0, Z = 3.0 },
    History = [new Sample { Seq = 1, Value = 100.5 }, new Sample { Seq = 2, Value = 200.25 }],
    Samples = [1, 2, 3, 5, 8, 13],
    RecentCodes = [10, 20, 30, 40],
    Tags = ["alpha", "beta", "gamma"],
    Matrix = [[1, 2, 3], [4, 5], [6]],
    Counters = [new() { Key = "frames", Value = 7 },
                new() { Key = "drops", Value = 1 },
                new() { Key = "resyncs", Value = 0 }],
    Reading = new Reading { _d = Mode.MODE_ACTIVE, ActiveRate = 99.5 },
    Window = [11, 22, 33, 44],
    Calibration = calibration,
};

static void AssertSample(Telemetry expected, Telemetry actual)
{
    Require(actual.UnitId == expected.UnitId, "unitId differs");
    Require(actual.Region == expected.Region, "region differs");
    Require(actual.Mode == expected.Mode, "mode differs");
    Require(actual.BatteryVolts == expected.BatteryVolts, "batteryVolts differs");
    Require(actual.BatteryCurrent == expected.BatteryCurrent, "transparent typedef differs");
    Require(actual.Location is not null &&
        actual.Location.X == expected.Location.X &&
        actual.Location.Y == expected.Location.Y &&
        actual.Location.Z == expected.Location.Z, "nested position differs");
    Require(actual.History is not null && actual.History.Length == expected.History.Length &&
        actual.History.Zip(expected.History, (a, b) =>
            a is not null && a.Seq == b.Seq && a.Value == b.Value).All(equal => equal),
        "struct sequence differs");
    Require(actual.Samples is not null && actual.Samples.SequenceEqual(expected.Samples),
        "unbounded sequence differs");
    Require(actual.RecentCodes is not null && actual.RecentCodes.SequenceEqual(expected.RecentCodes),
        "bounded sequence differs");
    Require(actual.Tags is not null && actual.Tags.SequenceEqual(expected.Tags), "string sequence differs");
    Require(actual.Matrix is not null && actual.Matrix.Length == expected.Matrix.Length &&
        actual.Matrix.Zip(expected.Matrix, (a, b) => a is not null && a.SequenceEqual(b)).All(equal => equal),
        "nested sequence differs");
    Require(actual.Counters is not null && actual.Counters.Length == expected.Counters.Length &&
        actual.Counters.All(entry => entry is not null) &&
        actual.Counters.Select(entry => (entry.Key, entry.Value)).OrderBy(entry => entry.Key)
            .SequenceEqual(expected.Counters.Select(entry => (entry.Key, entry.Value)).OrderBy(entry => entry.Key)),
        "map differs, including duplicate/missing keys");
    var reading = actual.Reading ?? throw new InvalidOperationException("union is null");
    Require(reading._d == expected.Reading._d, "union discriminator differs");
    var branchMatches = expected.Reading._d switch
    {
        Mode.MODE_IDLE => reading.IdleTicks == expected.Reading.IdleTicks,
        Mode.MODE_ACTIVE => reading.ActiveRate == expected.Reading.ActiveRate,
        _ => reading.FaultCode == expected.Reading.FaultCode,
    };
    Require(branchMatches, "selected union branch differs");
    Require(actual.Window is not null && actual.Window.SequenceEqual(expected.Window), "fixed array differs");
    Require(actual.Calibration == expected.Calibration, "optional presence or value differs");
}

if (args.Length != 2 || args[0] != "--domain" || !uint.TryParse(args[1], out var domain))
    throw new ArgumentException("usage: idl-conformance-csharp --domain INTEGER");
Console.WriteLine($"effective domain={domain}");

// Separate construction prevents a mutable-class shallow copy from sharing the
// expected sample's nested data with the second publication.
var present = MakeSample(42, 0.0078125);
var absent = MakeSample(43, null);
absent.Mode = Mode.MODE_FAULT;
absent.Reading = new Reading { _d = Mode.MODE_FAULT, FaultCode = "sensor-offline" };
Require(!ReferenceEquals(present, absent) && !ReferenceEquals(present.Location, absent.Location) &&
    !ReferenceEquals(present.History, absent.History), "samples must be independently constructed");
Require(present.Calibration.HasValue && absent.Calibration is null, "invalid optional test inputs");

using var participant = DomainParticipantFactory.Instance.CreateParticipant(domain);
using var publisher = participant.CreatePublisher();
using var subscriber = participant.CreateSubscriber();
using var topic = participant.CreateTypedTopic<Telemetry>("ConfCombo");
using var reader = subscriber.CreateTypedReader<Telemetry>(topic);
using var writer = publisher.CreateTypedWriter<Telemetry>(topic);
writer.WaitForMatched(1, Duration.FromSeconds(5));
reader.WaitForMatched(1, Duration.FromSeconds(5));

foreach (var sent in new[] { present, absent })
{
    writer.Write(sent);
    Require(reader.WaitForData(TimeSpan.FromSeconds(5)), $"timeout for unitId={sent.UnitId}");
    var received = reader.Take().Where(sample => sample.Info.ValidData).Select(sample => sample.Data).ToArray();
    Require(received.Length == 1, $"expected one valid sample for unitId={sent.UnitId}, got {received.Length}");
    var got = received[0] ?? throw new InvalidOperationException("valid sample has null data");
    AssertSample(sent, got);
    Console.WriteLine($"SAMPLE PASS unitId={sent.UnitId} optional={(got.Calibration.HasValue ? "present" : "absent")}");
}
Console.WriteLine("ROUNDTRIP PASS: both independent samples and every combined feature survived native DCPS.");
