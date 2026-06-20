// Snippet verbatim from website/topics/dev-install/index.html (RuntimeConfig tuning).
use std::time::Duration;
use zerodds_dcps::runtime::RuntimeConfig;
use zerodds_dcps::{DomainParticipantFactory, DomainParticipantQos};

fn main() -> zerodds_dcps::Result<()> {
    let factory = DomainParticipantFactory::instance();

    let cfg = RuntimeConfig {
        tick_period: Duration::from_millis(20),
        spdp_period: Duration::from_millis(100),
        ..RuntimeConfig::default()
    };
    let participant =
        factory.create_participant_with_config(0, DomainParticipantQos::default(), cfg)?;

    let _ = participant;
    Ok(())
}
