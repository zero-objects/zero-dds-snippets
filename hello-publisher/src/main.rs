//! Hello-World publisher — matches the README quickstart exactly.
//! Builds + runs against the published zerodds-dcps 1.0.0-rc.3.
use zerodds_dcps::*;

fn main() {
    let factory = DomainParticipantFactory::instance();
    let participant = factory.create_participant_offline(0, DomainParticipantQos::default());
    let topic = participant
        .create_topic::<RawBytes>("Greetings", TopicQos::default())
        .expect("create_topic");
    let writer = participant
        .create_publisher(PublisherQos::default())
        .create_datawriter::<RawBytes>(&topic, DataWriterQos::default())
        .expect("create_datawriter");

    writer.write(&RawBytes::new(b"Hello, DDS!".to_vec())).expect("write");
    println!("published: Hello, DDS!");
}
