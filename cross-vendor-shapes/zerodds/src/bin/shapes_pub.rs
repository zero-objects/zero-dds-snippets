//! ShapesDemo publisher on the four-member @final `ShapeType`.
//!   shapes_pub <topic> <color> <domain>
//! Writes one moving shape every 100 ms until it is stopped.
#![allow(non_snake_case, unused_imports, dead_code, clippy::all)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/ShapeType.rs"));
}
use generated::ShapeType;
use std::time::Duration;
use zerodds_dcps::{DataWriterQos, DomainParticipantFactory, DomainParticipantQos, PublisherQos, TopicQos};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = std::env::args().collect();
    let topic_name = args.get(1).map_or("Square", String::as_str);
    let color = args.get(2).map_or("BLUE", String::as_str);
    let domain: i32 = args.get(3).and_then(|s| s.parse().ok()).unwrap_or(0);
    let p = DomainParticipantFactory::instance().create_participant(domain, DomainParticipantQos::default())?;
    let topic = p.create_topic::<ShapeType>(topic_name, TopicQos::default())?;
    let w = p
        .create_publisher(PublisherQos::default())
        .create_datawriter::<ShapeType>(&topic, DataWriterQos::default())?;
    println!("[zerodds-pub] topic={topic_name} color={color} domain={domain}");
    for i in 0u32.. {
        let s = ShapeType {
            color: color.to_string(),
            x: 120 + ((i * 11) % 120) as i32,
            y: 225 - ((i * 2) % 100) as i32,
            shapesize: 30,
        };
        w.write(&s)?;
        println!("  -> color={} x={} y={}", s.color, s.x, s.y);
        std::thread::sleep(Duration::from_millis(100));
    }
    Ok(())
}
