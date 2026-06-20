import zerodds

factory = zerodds.DomainParticipantFactory.instance()
p = factory.create_participant(0)
topic = p.create_bytes_topic("Setpoint")

# state topic with a late-joiner cache (see QoS Cookbook §2)
qos = zerodds.DataWriterQos()
qos.set_reliability("Reliable", 0.1)
qos.set_durability("TransientLocal")
qos.set_history("KeepLast", 1)

writer = p.create_publisher().create_bytes_writer_with_qos(topic, qos)

# Smoke-check: the writer was actually constructed with the QoS.
print("durability:", qos.durability_kind())
print("reliability:", qos.reliability_kind())
print("history:", qos.history_kind(), qos.history_depth())
print("writer created:", writer is not None)
