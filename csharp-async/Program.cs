// async/await demo (website §"async/await").
//
// The load-bearing lines are the EXACT website snippet:
//
//     await writer.WriteAsync(payload, ct);
//     await reader.WaitForDataAsync(TimeSpan.FromSeconds(3), ct);
//     await foreach (var sample in reader.TakeAsync(ct))
//     {
//         Console.WriteLine(sample);
//     }

using ZeroDDS;
using ZeroDDS.Pub;
using ZeroDDS.Sub;
using ZeroDDS.Topic;

var factory = DomainParticipantFactory.Instance;
var participant = factory.CreateParticipant(7);

using var topic = new Topic<byte[]>(participant, "AsyncChatter", new ByteSeqTraits());
using var publisher = new Publisher(participant);
using var subscriber = new Subscriber(participant);

using var writer = new DataWriter<byte[]>(publisher, topic);
using var reader = new DataReader<byte[]>(subscriber, topic);

// Plumbing (not part of the snippet): let discovery match so the same-process
// loopback actually delivers, and bound the demo so it always terminates.
writer.WaitForMatched(1, ZeroDDS.Core.Duration.FromSeconds(5));
reader.WaitForMatched(1, ZeroDDS.Core.Duration.FromSeconds(5));

var payload = "ping"u8.ToArray();
using var cts = new CancellationTokenSource(TimeSpan.FromSeconds(10));
var ct = cts.Token;

// ---- EXACT website snippet ----
await writer.WriteAsync(payload, ct);
await reader.WaitForDataAsync(TimeSpan.FromSeconds(3), ct);
await foreach (var sample in reader.TakeAsync(ct))
{
    Console.WriteLine(sample);
}
// ---- end snippet ----

Console.WriteLine("async demo OK");
