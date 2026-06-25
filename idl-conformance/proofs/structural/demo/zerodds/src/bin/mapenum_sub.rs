#![allow(non_snake_case, unused_imports, clippy::all)]
mod generated { include!(concat!(env!("OUT_DIR"), "/mapenum.rs")); }
use generated::feat::{MapEnum, Pt, Sel, Hue};
use std::time::Duration;
use zerodds_dcps::{DataReaderQos, DdsError, DomainParticipantFactory, DomainParticipantQos, SubscriberQos, TopicQos};
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let domain: i32 = std::env::args().nth(1).and_then(|s| s.parse().ok()).unwrap_or(0);
    let f = DomainParticipantFactory::instance();
    let p = f.create_participant(domain, DomainParticipantQos::default())?;
    let topic = p.create_topic::<MapEnum>("MapEnum", TopicQos::default())?;
    let mut q = DataReaderQos::default(); q.reliability.kind = zerodds_dcps::qos::ReliabilityKind::BestEffort;
    let r = p.create_subscriber(SubscriberQos::default()).create_datareader::<MapEnum>(&topic, q)?;
    loop { match r.wait_for_data(Duration::from_secs(1)) {
        Ok(()) => for s in r.take()? {
            let ok = s.h==Hue::H_BLUE && s.m.get(&3)==Some(&Pt{x:11,y:12}) && s.sels==vec![Sel::N(9)];
            println!("  <- h={:?} m.len={} sels.len={} integrity={}", s.h, s.m.len(), s.sels.len(), if ok {"OK"} else {"BAD"});
        },
        Err(DdsError::Timeout) => {}, Err(e) => return Err(e.into()),
    } }
}
