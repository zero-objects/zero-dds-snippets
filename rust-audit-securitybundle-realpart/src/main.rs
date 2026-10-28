// audit-reference block 3/7.
//
// The website snippet, verified end to end: the plugin construction
//   StderrLoggingPlugin::with_level / JsonLinesLoggingPlugin::open /
//   FanOutLoggingPlugin::new().with(...) / SecurityBundle::builder()
//     .logging_plugin(Box::new(fanout)).build()
// and the participant wiring
//     DomainParticipantFactory::create(0).with_security(bundle).build()
// (`zerodds-dcps` feature `security`). The same participant comes from
// RuntimeConfig::with_security_bundle + create_participant_with_config,
// which is what the builder does underneath.
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

    let participant = DomainParticipantFactory::create(0)
        .with_security(security_bundle)
        .build()?;

    let _ = participant;
    println!("security bundle wired, audit ndjson at {}", audit_path.display());
    Ok(())
}
