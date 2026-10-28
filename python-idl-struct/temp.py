import zerodds

# temperature.py is the `zerodds-idlc generate --lang python temperature.idl`
# output: the dataclass, its codec over zerodds_wire, ZD_TOPIC_DESCRIPTORS.
from temperature import ZD_TOPIC_DESCRIPTORS

factory = zerodds.DomainParticipantFactory.instance()
p = factory.create_participant(0)

topic = zerodds.IdlTopic(p, "Temp", ZD_TOPIC_DESCRIPTORS["sensor_msgs::msg::Temperature"])
Temperature = topic.cls
writer = topic.create_writer(p.create_publisher())
reader = topic.create_reader(p.create_subscriber())

writer.wait_for_matched_subscription(1, 5.0)
reader.wait_for_matched_publication(1, 5.0)

writer.write(Temperature(celsius=23, sensor_id="A7", raw_blob=[]))
reader.wait_for_data(3.0)
for msg in reader.take():
    print(msg)
# sensor_msgs.msg.Temperature(celsius=23, sensor_id='A7', raw_blob=[])
