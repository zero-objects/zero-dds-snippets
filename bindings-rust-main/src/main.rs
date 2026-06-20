use std::time::Duration;
use zerodds_dcps::*;

fn main() -> Result<()> {
    let factory = DomainParticipantFactory::instance();
    let p = factory.create_participant(0, DomainParticipantQos::default())?;

    let topic = p.create_topic::<RawBytes>("Chatter", TopicQos::default())?;
    let writer = p.create_publisher(PublisherQos::default())
        .create_datawriter::<RawBytes>(&topic, DataWriterQos::default())?;
    let reader = p.create_subscriber(SubscriberQos::default())
        .create_datareader::<RawBytes>(&topic, DataReaderQos::default())?;

    writer.wait_for_matched_subscription(1, Duration::from_secs(5))?;
    reader.wait_for_matched_publication(1, Duration::from_secs(5))?;

    writer.write(&RawBytes::new(b"hello".to_vec()))?;
    reader.wait_for_data(Duration::from_secs(3))?;

    for sample in reader.take()? {
        println!("got: {:?}", sample.data);
    }
    Ok(())
}
