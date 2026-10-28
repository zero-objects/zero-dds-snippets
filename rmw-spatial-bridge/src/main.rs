//! The SpatialDDS side of the `ros2.spatial_bridge` example: writes or
//! takes `spatial::core::FramedPose` on `sdds/default/FramedPose`, the topic
//! the bridge inside a ROS 2 node (`rmw_zerodds_cpp`) maps to `/robot1/pose`.
//!
//!   spatial_side pub SECONDS   writes a FramedPose every 100 ms
//!   spatial_side sub SECONDS   prints every FramedPose it takes
//!
//! The domain is `ROS_DOMAIN_ID`, the ROS node's.
use std::time::{Duration, Instant};

use rmw_zerodds::spatial_bridge::BridgeFramedPose;
use zerodds_dcps::qos::{
    DurabilityKind, DurabilityQosPolicy, HistoryKind, HistoryQosPolicy, ReliabilityKind,
    ReliabilityQosPolicy,
};
use zerodds_dcps::{
    DataReaderQos, DataWriterQos, DomainParticipantFactory, DomainParticipantQos, PublisherQos,
    SubscriberQos, TopicQos,
};
use zerodds_spatial_dds::builtin::Time;
use zerodds_spatial_dds::spatial::common::FrameRef;
use zerodds_spatial_dds::spatial::core::{FramedPose, PoseSE3};

const TOPIC: &str = "sdds/default/FramedPose";

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = std::env::args().collect();
    let mode = args.get(1).map_or("pub", String::as_str);
    let seconds: u64 = args.get(2).and_then(|s| s.parse().ok()).unwrap_or(10);
    let domain: i32 = std::env::var("ROS_DOMAIN_ID")
        .ok()
        .and_then(|d| d.parse().ok())
        .unwrap_or(0);
    let participant = DomainParticipantFactory::instance()
        .create_participant(domain, DomainParticipantQos::default())?;
    let topic = participant.create_topic::<BridgeFramedPose>(TOPIC, TopicQos::default())?;
    // The bridge's own QoS: RELIABLE, KEEP_LAST 10.
    let reliability = ReliabilityQosPolicy {
        kind: ReliabilityKind::Reliable,
        ..Default::default()
    };
    let history = HistoryQosPolicy {
        kind: HistoryKind::KeepLast,
        depth: 10,
    };
    let deadline = Instant::now() + Duration::from_secs(seconds);
    let mut n: i32 = 0;
    if mode == "pub" {
        let writer = participant
            .create_publisher(PublisherQos::default())
            .create_datawriter(
                &topic,
                DataWriterQos {
                    reliability,
                    history,
                    // The SpatialDDS codecs have only an XCDR2 form, as the
                    // bridge's own participant says (`[XCDR2]`); the
                    // runtime's default offer is XCDR1 first.
                    data_representation: Some(vec![
                        zerodds_rtps::publication_data::data_representation::XCDR2,
                    ]),
                    ..Default::default()
                },
            )?;
        // A VOLATILE writer's samples reach the readers it has matched: wait
        // for the bridge's reader before the first write, or the poses
        // written in the meantime go to nobody.
        writer.wait_for_matched_subscription(1, Duration::from_secs(seconds))?;
        while Instant::now() < deadline {
            n += 1;
            writer.write(&BridgeFramedPose(FramedPose {
                pose: PoseSE3 {
                    t: [f64::from(n), 2.0, 3.0],
                    q: [0.0, 0.0, 0.0, 1.0],
                },
                frame_ref: FrameRef {
                    fqn: "map".to_string(),
                    ..Default::default()
                },
                cov: Default::default(),
                stamp: Time { sec: n, nanosec: 0 },
            }))?;
            std::thread::sleep(Duration::from_millis(100));
        }
        println!("spatial_side: wrote {n} FramedPose");
    } else {
        let reader = participant
            .create_subscriber(SubscriberQos::default())
            .create_datareader(
                &topic,
                DataReaderQos {
                    reliability,
                    history,
                    // The bridge's writer offers TRANSIENT_LOCAL with its
                    // KEEP_LAST 10: a pose it forwarded before it matched
                    // this reader reaches it from that history (DDS 1.4
                    // §2.2.3.4). A VOLATILE reader would not get it.
                    durability: DurabilityQosPolicy {
                        kind: DurabilityKind::TransientLocal,
                    },
                    ..Default::default()
                },
            )?;
        while Instant::now() < deadline {
            for sample in reader.take()? {
                let pose = &sample.0;
                n += 1;
                println!(
                    "spatial_side: FramedPose frame={} t={:?}",
                    pose.frame_ref.fqn, pose.pose.t
                );
            }
            std::thread::sleep(Duration::from_millis(50));
        }
        println!("spatial_side: took {n} FramedPose");
    }
    Ok(())
}
