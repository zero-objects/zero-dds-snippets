//! OMG Time Service 1.1 as the DCPS time source: SOURCE_TIMESTAMP comes
//! from a `TimeService`, not from the system clock.
//!
//! The service here reads the system clock one hour ahead — a stand-in for
//! a disciplined clock (GNSS, PTP) that a `TimeService::with_source`
//! closure would read. A sample written and taken in one participant
//! carries that clock's time.
use std::sync::Arc;
use std::time::{Duration, Instant};

use zerodds_dcps::dds_type::RawBytes;
use zerodds_dcps::runtime::RuntimeConfig;
use zerodds_dcps::time_source::{TimeServiceClock, time_to_ticks};
use zerodds_dcps::{
    DataReaderQos, DataWriterQos, DomainParticipantFactory, DomainParticipantQos, PublisherQos,
    SubscriberQos, TopicQos,
};
use zerodds_time_service::TimeService;

/// One hour in the Time Service's 100 ns ticks.
const AHEAD: u64 = 3600 * 10_000_000;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let domain: i32 = std::env::var("ZERODDS_EXAMPLE_DOMAIN")
        .ok()
        .and_then(|d| d.parse().ok())
        .unwrap_or(0);
    let service = TimeService::with_source(|| {
        time_to_ticks(zerodds_dcps::time::get_current_time()) + AHEAD
    });
    let config = RuntimeConfig {
        time_source: Arc::new(TimeServiceClock::new(service)),
        ..RuntimeConfig::from_standard_paths()?
    };
    let participant = DomainParticipantFactory::instance().create_participant_with_config(
        domain,
        DomainParticipantQos::default(),
        config,
    )?;
    let topic = participant.create_topic::<RawBytes>("TimeServiceClock", TopicQos::default())?;
    let reader = participant
        .create_subscriber(SubscriberQos::default())
        .create_datareader(&topic, DataReaderQos::default())?;
    let writer = participant
        .create_publisher(PublisherQos::default())
        .create_datawriter(&topic, DataWriterQos::default())?;

    writer.write(&RawBytes::new(b"stamped by the time service".to_vec()))?;
    let deadline = Instant::now() + Duration::from_secs(5);
    loop {
        if let Some(sample) = reader.take_with_info()?.into_iter().next() {
            let stamped = sample.info.source_timestamp;
            let now = zerodds_dcps::time::get_current_time();
            let ahead = i64::from(stamped.sec) - i64::from(now.sec);
            println!(
                "source timestamp {}.{:09} s, system clock {}.{:09} s: {ahead:+} s",
                stamped.sec, stamped.nanosec, now.sec, now.nanosec
            );
            if (3599..=3600).contains(&ahead) {
                println!("SOURCE_TIMESTAMP follows the time service");
                return Ok(());
            }
            return Err("the source timestamp is not the time service's".into());
        }
        if Instant::now() > deadline {
            return Err("no sample within 5 s".into());
        }
        std::thread::sleep(Duration::from_millis(20));
    }
}
