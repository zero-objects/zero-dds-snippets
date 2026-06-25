#![allow(non_snake_case, unused_imports, clippy::all)]
mod generated { include!(concat!(env!("OUT_DIR"), "/rich.rs")); }
use generated::{RichF, RichA, RichM};
use std::time::Duration;
use zerodds_dcps::{DataWriterQos, DomainParticipantFactory, DomainParticipantQos, PublisherQos, TopicQos};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let domain: i32 = std::env::args().nth(1).and_then(|s| s.parse().ok()).unwrap_or(0);
    let ext = std::env::args().nth(2).unwrap_or_else(|| "m".into());
    let blob_size: usize = std::env::args().nth(3).and_then(|s| s.parse().ok()).unwrap_or(50);
    let f = DomainParticipantFactory::instance();
    let p = f.create_participant(domain, DomainParticipantQos::default())?;
    let pubr = p.create_publisher(PublisherQos::default());

    macro_rules! run { ($T:ident, $topic:expr) => {{
        let topic = p.create_topic::<$T>($topic, TopicQos::default())?;
        let w = pubr.create_datawriter::<$T>(&topic, DataWriterQos::default())?;
        let _ = w.wait_for_matched_subscription(1, Duration::from_secs(10));
        println!("[rich-pub] domain={domain} ext={ext} topic={} blob={blob_size}", $topic);
        let mut id = 0u32;
        loop {
            id += 1;
            let blob: Vec<u8> = (0..blob_size).map(|i| (i % 251) as u8).collect();
            let s = $T { id, value: 3.141592653589793_f64 * id as f64, blob };
            w.write(&s)?;
            println!("  -> id={id} value={:.3} blob={}", s.value, blob_size);
            std::thread::sleep(Duration::from_millis(150));
        }
    }}}

    match ext.as_str() {
        "f" => run!(RichF, "RichF"),
        "a" => run!(RichA, "RichA"),
        _   => run!(RichM, "RichM"),
    }
}
