use std::time::Duration;
use zerodds_dcps_async::*;
use zerodds_dcps::RawBytes;

#[tokio::main]
async fn main() -> Result<()> {
    let factory = AsyncDomainParticipantFactory::instance();
    let p = factory.create_participant(0)?;

    let topic = p.create_topic::<RawBytes>("Chatter", TopicQos::default())?;
    let writer = p.create_publisher(PublisherQos::default())
        .create_datawriter::<RawBytes>(&topic, DataWriterQos::default())?;
    let reader = p.create_subscriber(SubscriberQos::default())
        .create_datareader::<RawBytes>(&topic, DataReaderQos::default())?;

    writer.wait_for_matched_subscription(1, Duration::from_secs(5)).await?;
    reader.wait_for_matched_publication(1, Duration::from_secs(5)).await?;

    writer.write(&RawBytes::new(b"hello".to_vec())).await?;

    let samples = reader.take(Duration::from_secs(3)).await?;
    for s in samples {
        println!("got: {:?}", s.data);
    }
    Ok(())
}
