// audit-reference block 3/7 — the REAL portion only.
//
// The website snippet's plugin construction is real and verified here:
//   StderrLoggingPlugin::with_level / JsonLinesLoggingPlugin::open /
//   FanOutLoggingPlugin::new().with(...) / SecurityBundle::builder()
//     .logging_plugin(Box::new(fanout)).build()
//
// What the website snippet gets WRONG (kept out of this demo, surfaced as a
// question) is the participant wiring:
//     DomainParticipantFactory::create(0).with_security(bundle).build()
// None of `create`, `with_security`, `build` exist on the factory. The real
// wiring is shown below: RuntimeConfig::with_security_bundle + a
// create_participant_with_config call.
use zerodds_dcps::qos::DomainParticipantQos;
use zerodds_dcps::runtime::RuntimeConfig;
use zerodds_dcps::DomainParticipantFactory;
use zerodds_security::logging::LogLevel;
use zerodds_security_logging::{FanOutLoggingPlugin, JsonLinesLoggingPlugin, StderrLoggingPlugin};
use zerodds_security_runtime::SecurityBundle;

fn main() -> zerodds_dcps::Result<()> {
    let stderr_sink = StderrLoggingPlugin::with_level(LogLevel::Warning);
    let audit_path = std::env::temp_dir().join("zerodds-audit-demo.ndjson");
    let audit_sink = JsonLinesLoggingPlugin::open(&audit_path, LogLevel::Notice)
        .expect("open jsonl audit sink");

    let fanout = FanOutLoggingPlugin::new().with(stderr_sink).with(audit_sink);

    let security_bundle = SecurityBundle::builder()
        .logging_plugin(Box::new(fanout))
        // ... auth, access, crypto plugins ...
        .build();

    assert!(security_bundle.has_logging());

    // REAL participant wiring (what the website snippet should show):
    let cfg = RuntimeConfig::default().with_security_bundle(&security_bundle);
    let factory = DomainParticipantFactory::instance();
    let participant =
        factory.create_participant_with_config(0, DomainParticipantQos::default(), cfg)?;

    let _ = participant;
    println!("security bundle wired, audit ndjson at {}", audit_path.display());
    Ok(())
}
