use std::time::Duration;
use zerodds_dcps::runtime::RuntimeConfig;
use zerodds_dcps::*;

fn main() {
    // Two participants in one process, same domain, exchanging over UDP loopback.
    let cfg = RuntimeConfig {
        tick_period: Duration::from_millis(20),
        spdp_period: Duration::from_millis(100),
        ..RuntimeConfig::default()
    };
    let factory = DomainParticipantFactory::instance();
    let domain = 0;

    let pub_p = factory
        .create_participant_with_config(domain, DomainParticipantQos::default(), cfg.clone())
        .expect("pub participant");
    let sub_p = factory
        .create_participant_with_config(domain, DomainParticipantQos::default(), cfg)
        .expect("sub participant");

    let pub_topic = pub_p.create_topic::<RawBytes>("Chatter", TopicQos::default()).expect("pub topic");
    let sub_topic = sub_p.create_topic::<RawBytes>("Chatter", TopicQos::default()).expect("sub topic");

    let writer = pub_p
        .create_publisher(PublisherQos::default())
        .create_datawriter::<RawBytes>(&pub_topic, DataWriterQos::default())
        .expect("writer");
    let reader = sub_p
        .create_subscriber(SubscriberQos::default())
        .create_datareader::<RawBytes>(&sub_topic, DataReaderQos::default())
        .expect("reader");

    // Discovery handshake (event-driven, no busy-poll).
    writer.wait_for_matched_subscription(1, Duration::from_secs(5)).expect("writer matched");
    reader.wait_for_matched_publication(1, Duration::from_secs(5)).expect("reader matched");

    writer.write(&RawBytes::new(b"Hello, DDS!".to_vec())).expect("write");

    reader.wait_for_data(Duration::from_secs(3)).expect("sample arrives");
    let samples = reader.take().expect("take");
    println!("received {} sample(s): {:?}", samples.len(), String::from_utf8_lossy(&samples[0].data));
}
