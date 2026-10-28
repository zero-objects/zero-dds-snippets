//! The DDS side of the web gateway example: an ordinary ZeroDDS program
//! with the type idlc generates from `idl/Sensor.idl`, the file the gateway
//! reads too.
//!
//! - `dds_side sub SECONDS` prints every `Demo::Sensor` on topic `Sensor`
//!   for SECONDS and then the count;
//! - `dds_side pub COUNT` waits for a reader, writes COUNT samples
//!   (`sensor_id` 100, 101, …, `value` = `sensor_id` / 10) and waits until
//!   they are acknowledged.
//!
//! Domain: `ZERODDS_EXAMPLE_DOMAIN` (default 0).
use std::time::{Duration, Instant};

use zerodds_dcps::{
    DataReaderQos, DataWriterQos, DomainParticipantFactory, DomainParticipantQos, PublisherQos,
    SubscriberQos, TopicQos,
};

#[allow(
    clippy::too_many_lines,
    clippy::useless_conversion,
    dead_code,
    unused_imports,
    non_snake_case
)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/Sensor.rs"));
}
use generated::Demo::Sensor;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut args = std::env::args().skip(1);
    let mode = args.next().unwrap_or_default();
    let n: u64 = args.next().and_then(|v| v.parse().ok()).unwrap_or(5);
    let domain: i32 = std::env::var("ZERODDS_EXAMPLE_DOMAIN")
        .ok()
        .and_then(|d| d.parse().ok())
        .unwrap_or(0);
    let participant = DomainParticipantFactory::instance()
        .create_participant(domain, DomainParticipantQos::default())?;
    let topic = participant.create_topic::<Sensor>("Sensor", TopicQos::default())?;
    match mode.as_str() {
        "sub" => {
            let reader = participant
                .create_subscriber(SubscriberQos::default())
                .create_datareader(&topic, DataReaderQos::default())?;
            let end = Instant::now() + Duration::from_secs(n);
            let mut count = 0;
            while Instant::now() < end {
                for s in reader.take()? {
                    println!("dds_side: Sensor sensor_id={} value={}", s.sensor_id, s.value);
                    count += 1;
                }
                std::thread::sleep(Duration::from_millis(20));
            }
            println!("dds_side: took {count} Sensor");
        }
        "pub" => {
            let writer = participant
                .create_publisher(PublisherQos::default())
                .create_datawriter(&topic, DataWriterQos::default())?;
            writer.wait_for_matched_subscription(1, Duration::from_secs(10))?;
            for i in 0..n {
                let id = 100 + i32::try_from(i)?;
                writer.write(&Sensor {
                    sensor_id: id,
                    value: f64::from(id) / 10.0,
                })?;
                std::thread::sleep(Duration::from_millis(100));
            }
            writer.wait_for_acknowledgments(Duration::from_secs(5))?;
            println!("dds_side: wrote {n} Sensor");
        }
        _ => return Err("usage: dds_side sub SECONDS | pub COUNT".into()),
    }
    Ok(())
}
