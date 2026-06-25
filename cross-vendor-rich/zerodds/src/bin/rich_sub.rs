#![allow(non_snake_case, unused_imports, clippy::all)]
mod generated { include!(concat!(env!("OUT_DIR"), "/rich.rs")); }
use generated::{RichF, RichA, RichM};
use std::time::Duration;
use zerodds_dcps::{DataReaderQos, DdsError, DomainParticipantFactory, DomainParticipantQos, SubscriberQos, TopicQos};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let domain: i32 = std::env::args().nth(1).and_then(|s| s.parse().ok()).unwrap_or(0);
    let ext = std::env::args().nth(2).unwrap_or_else(|| "m".into());
    let f = DomainParticipantFactory::instance();
    let p = f.create_participant(domain, DomainParticipantQos::default())?;
    let sub = p.create_subscriber(SubscriberQos::default());
    let mut q = DataReaderQos::default();
    q.reliability.kind = if std::env::var("ZD_BE").is_ok() {
        eprintln!("[cfg] BEST_EFFORT reader");
        zerodds_dcps::qos::ReliabilityKind::BestEffort
    } else { zerodds_dcps::qos::ReliabilityKind::Reliable };

    macro_rules! run { ($T:ident, $topic:expr) => {{
        let topic = p.create_topic::<$T>($topic, TopicQos::default())?;
        let r = sub.create_datareader::<$T>(&topic, q)?;
        println!("[rich-sub] domain={domain} ext={ext} topic={}", $topic);
        loop {
            match r.wait_for_data(Duration::from_secs(1)) {
                Ok(()) => match r.take() {
                    Ok(samples) => { for s in samples {
                        let ok = s.blob.iter().enumerate().all(|(i, b)| *b == (i % 251) as u8);
                        println!("  <- id={} value={:.3} blob={} integrity={}", s.id, s.value, s.blob.len(), if ok {"OK"} else {"CORRUPT"});
                    } }
                    Err(e) => eprintln!("[take] ERR: {e}"),
                },
                Err(DdsError::Timeout) => {
                    eprintln!("[disc] participants={} publications={} matched={}",
                        p.discovered_participants_count(), p.discovered_publications_count(), r.matched_publication_count());
                }
                Err(e) => return Err(e.into()),
            }
        }
    }}}

    match ext.as_str() {
        "f" => run!(RichF, "RichF"),
        "a" => run!(RichA, "RichA"),
        _   => run!(RichM, "RichM"),
    }
}
