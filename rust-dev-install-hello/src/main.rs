// Snippet verbatim from website/topics/dev-install/index.html (hello world).
use std::time::Duration;
use zerodds_dcps::*;

fn main() -> Result<()> {
    let factory = DomainParticipantFactory::instance();
    let participant = factory.create_participant(0, DomainParticipantQos::default())?;

    let topic = participant.create_topic::<RawBytes>("Chatter", TopicQos::default())?;
    let publisher = participant.create_publisher(PublisherQos::default());
    let subscriber = participant.create_subscriber(SubscriberQos::default());

    let writer = publisher.create_datawriter::<RawBytes>(&topic, DataWriterQos::default())?;
    let reader = subscriber.create_datareader::<RawBytes>(&topic, DataReaderQos::default())?;

    writer.wait_for_matched_subscription(1, Duration::from_secs(5))?;
    reader.wait_for_matched_publication(1, Duration::from_secs(5))?;

    writer.write(&RawBytes::new(b"hello".to_vec()))?;
    reader.wait_for_data(Duration::from_secs(3))?;

    for sample in reader.take()? {
        println!("got: {:?}", sample.data);
    }
    Ok(())
}
