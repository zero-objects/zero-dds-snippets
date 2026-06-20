using ZeroDDS;

var factory = DomainParticipantFactory.Instance;
var participant = factory.CreateParticipant(0);

var topic = participant.CreateBytesTopic("Chatter");
var publisher = participant.CreatePublisher();
var subscriber = participant.CreateSubscriber();

using var writer = publisher.CreateBytesWriter(topic);
using var reader = subscriber.CreateBytesReader(topic);

writer.WaitForMatchedSubscription(1, TimeSpan.FromSeconds(5));
reader.WaitForMatchedPublication(1, TimeSpan.FromSeconds(5));

writer.Write("hello"u8.ToArray());
reader.WaitForData(TimeSpan.FromSeconds(3));
foreach (var payload in reader.Take())
{
    Console.WriteLine($"got: {System.Text.Encoding.UTF8.GetString(payload)}");
}
