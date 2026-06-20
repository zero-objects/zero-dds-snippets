from dataclasses import dataclass
import zerodds
from zerodds.idl import idl_struct, Int32, String, Bytes

@idl_struct(typename="sensor_msgs::msg::Temperature")
@dataclass
class Temperature:
    celsius: Int32
    sensor_id: String
    raw_blob: Bytes = b""

factory = zerodds.DomainParticipantFactory.instance()
p = factory.create_participant(0)

topic = zerodds.IdlTopic(p, "Temp", Temperature)
writer = topic.create_writer(p.create_publisher())
reader = topic.create_reader(p.create_subscriber())

writer.wait_for_matched_subscription(1, 5.0)
reader.wait_for_matched_publication(1, 5.0)

writer.write(Temperature(celsius=23, sensor_id="A7"))
reader.wait_for_data(3.0)
for msg in reader.take():
    print(msg)
# Temperature(celsius=23, sensor_id='A7', raw_blob=b'')
