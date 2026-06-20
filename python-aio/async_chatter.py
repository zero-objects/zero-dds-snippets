import asyncio
import zerodds
from zerodds import aio as zaio

async def main():
    factory = zerodds.DomainParticipantFactory.instance()
    p = factory.create_participant(0)
    topic = p.create_bytes_topic("Chatter")

    writer = zaio.AsyncBytesWriter(p.create_publisher().create_bytes_writer(topic))
    reader = zaio.AsyncBytesReader(p.create_subscriber().create_bytes_reader(topic))

    await writer.wait_for_matched_subscription(1, 5.0)
    await reader.wait_for_matched_publication(1, 5.0)

    await writer.write(b"async hello")
    await reader.wait_for_data(3.0)
    for payload in reader.take():
        print(payload)

asyncio.run(main())
