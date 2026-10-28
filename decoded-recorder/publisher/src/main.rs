//! Publisher of `telemetry::Reading` for the decoded-recorder workflow.
//!   reading_pub <domain> <count>
//! Waits for a reader, then writes `count` samples, one every 100 ms.
#![allow(non_snake_case, unused_imports, dead_code, clippy::all)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/sensor.rs"));
}
use generated::telemetry::{Reading, Unit};
use std::time::Duration;
use zerodds_dcps::{
    DataWriterQos, DomainParticipantFactory, DomainParticipantQos, PublisherQos, TopicQos,
};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = std::env::args().collect();
    let domain: i32 = args.get(1).and_then(|s| s.parse().ok()).unwrap_or(0);
    let count: i32 = args.get(2).and_then(|s| s.parse().ok()).unwrap_or(20);
    let p = DomainParticipantFactory::instance()
        .create_participant(domain, DomainParticipantQos::default())?;
    let topic = p.create_topic::<Reading>("Readings", TopicQos::default())?;
    let w = p
        .create_publisher(PublisherQos::default())
        .create_datawriter::<Reading>(&topic, DataWriterQos::default())?;
    // The spy and the recorder both read; a sample written before they
    // matched would be lost to a VOLATILE reader.
    w.wait_for_matched_subscription(2, Duration::from_secs(8))?;
    println!(
        "[reading-pub] domain={domain} matched={}",
        w.matched_subscription_count()
    );
    for i in 0..count {
        let r = Reading {
            sensor_id: 7 + i % 3,
            name: format!("intake-{i}"),
            unit: match i % 3 {
                0 => Unit::CELSIUS,
                1 => Unit::KELVIN,
                _ => Unit::FAHRENHEIT,
            },
            value: 21.5 + f64::from(i),
            history: vec![20 + i, 21 + i, 22 + i],
        };
        w.write(&r)?;
        println!("  -> sensor_id={} name={}", r.sensor_id, r.name);
        std::thread::sleep(Duration::from_millis(100));
    }
    // Reliable delivery of the last samples before the participant goes.
    std::thread::sleep(Duration::from_millis(500));
    Ok(())
}
