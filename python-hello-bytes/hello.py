import zerodds

factory = zerodds.DomainParticipantFactory.instance()
participant = factory.create_participant(0)

topic = participant.create_bytes_topic("Chatter")
publisher = participant.create_publisher()
subscriber = participant.create_subscriber()

writer = publisher.create_bytes_writer(topic)
reader = subscriber.create_bytes_reader(topic)

writer.wait_for_matched_subscription(1, timeout_secs=5.0)
reader.wait_for_matched_publication(1, timeout_secs=5.0)

writer.write(b"hello")
reader.wait_for_data(timeout_secs=3.0)
for payload in reader.take():
    print(f"got: {payload!r}")
