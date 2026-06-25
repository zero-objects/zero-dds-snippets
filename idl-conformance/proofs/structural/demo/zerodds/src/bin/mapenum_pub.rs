#![allow(non_snake_case, unused_imports, clippy::all)]
mod generated { include!(concat!(env!("OUT_DIR"), "/mapenum.rs")); }
use generated::feat::{MapEnum, Pt, Sel, Hue};
use std::collections::BTreeMap;
use std::time::Duration;
use zerodds_dcps::{DataWriterQos, DomainParticipantFactory, DomainParticipantQos, PublisherQos, TopicQos};
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let domain: i32 = std::env::args().nth(1).and_then(|s| s.parse().ok()).unwrap_or(0);
    let f = DomainParticipantFactory::instance();
    let p = f.create_participant(domain, DomainParticipantQos::default())?;
    let topic = p.create_topic::<MapEnum>("MapEnum", TopicQos::default())?;
    let w = p.create_publisher(PublisherQos::default()).create_datawriter::<MapEnum>(&topic, DataWriterQos::default())?;
    let _ = w.wait_for_matched_subscription(1, Duration::from_secs(10));
    let mut m = BTreeMap::new();
    m.insert(3i32, Pt { x: 11, y: 12 });
    let s = MapEnum { h: Hue::H_BLUE, m, sels: vec![Sel::N(9)] };
    println!("[zd-mapenum-pub] h=H_BLUE m={{3:(11,12)}} sels=[N(9)]");
    loop { w.write(&s)?; std::thread::sleep(Duration::from_millis(150)); }
}
