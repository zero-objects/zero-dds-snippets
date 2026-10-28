// Typed-topic demo (website §"Typed topics — idlc csharp" → "Codegen + use").
//
// The three load-bearing lines are the EXACT website snippet:
//
//     var topic = participant.CreateTypedTopic<Temperature>("Temp");
//     var writer = publisher.CreateTypedWriter<Temperature>(topic);
//     writer.Write(new Temperature { Celsius = 23, Sensor_id = "A7" });
//
// Temperature.cs and Temperature.TypeSupport.cs are the verbatim
// `zerodds-idlc generate --lang csharp temperature.idl` output.

using ZeroDDS;
using sensor_msgs.msg;

var factory = DomainParticipantFactory.Instance;
var participant = factory.CreateParticipant(0);
var publisher = participant.CreatePublisher();

var topic = participant.CreateTypedTopic<Temperature>("Temp");
var writer = publisher.CreateTypedWriter<Temperature>(topic);
writer.Write(new Temperature { Celsius = 23, Sensor_id = "A7" });

Console.WriteLine($"wrote Temperature {{ Celsius = 23, Sensor_id = \"A7\" }} on topic '{topic.Name}' (type {topic.TypeName})");
