//! ShapesDemo subscriber on the four-member @final `ShapeType`.
//!   shapes_sub <topic> <domain>
//! Prints one "<-" line per sample until it is stopped.
#![allow(non_snake_case, unused_imports, dead_code, clippy::all)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/ShapeType.rs"));
}
use generated::ShapeType;
use std::time::Duration;
use zerodds_dcps::{DataReaderQos, DdsError, DomainParticipantFactory, DomainParticipantQos, SubscriberQos, TopicQos};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = std::env::args().collect();
    let topic_name = args.get(1).map_or("Square", String::as_str);
    let domain: i32 = args.get(2).and_then(|s| s.parse().ok()).unwrap_or(0);
    let p = DomainParticipantFactory::instance().create_participant(domain, DomainParticipantQos::default())?;
    let topic = p.create_topic::<ShapeType>(topic_name, TopicQos::default())?;
    let r = p
        .create_subscriber(SubscriberQos::default())
        .create_datareader::<ShapeType>(&topic, DataReaderQos::default())?;
    println!("[zerodds-sub] topic={topic_name} domain={domain}");
    loop {
        match r.wait_for_data(Duration::from_secs(1)) {
            Ok(()) => {
                for s in r.take()? {
                    println!("  <- color={:8} x={:4} y={:4} size={}", s.color, s.x, s.y, s.shapesize);
                }
            }
            Err(DdsError::Timeout) => {}
            Err(e) => return Err(e.into()),
        }
    }
}
