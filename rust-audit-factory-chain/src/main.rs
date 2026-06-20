//! Demo for the `topics/audit-reference` website snippet: wiring a
//! `SecurityBundle` (with a fan-out audit logger) into a participant via the
//! fluent `DomainParticipantFactory::create(..).with_security(..).build()`
//! chain.
//!
//! `main` runs the chain for real (against a writable temp path);
//! [`website_snippet_exact`] holds the verbatim website code and exists purely
//! to prove the snippet compiles unchanged.

use zerodds_dcps::DomainParticipantFactory;
use zerodds_security::logging::LogLevel;
use zerodds_security_logging::{FanOutLoggingPlugin, JsonLinesLoggingPlugin, StderrLoggingPlugin};
use zerodds_security_runtime::SecurityBundle;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Writable audit path for the live run (the website snippet uses
    // /var/log/zerodds which needs root; the API is identical).
    let audit_path = std::env::temp_dir().join("zerodds-audit-demo.ndjson");

    let stderr_sink = StderrLoggingPlugin::with_level(LogLevel::Warning);
    let audit_sink = JsonLinesLoggingPlugin::open(&audit_path, LogLevel::Notice)?;

    let fanout = FanOutLoggingPlugin::new().with(stderr_sink).with(audit_sink);

    let security_bundle = SecurityBundle::builder()
        .logging_plugin(Box::new(fanout))
        // ... auth, access, crypto plugins ...
        .build();

    // The fluent chain under test.
    let participant = DomainParticipantFactory::create(0)
        .with_security(security_bundle)
        .build()?;

    println!(
        "participant up on domain {} — audit fan-out wired",
        participant.domain_id()
    );

    DomainParticipantFactory::instance().delete_participant(&participant)?;
    let _ = std::fs::remove_file(&audit_path);
    Ok(())
}

/// The exact code shown on `website/topics/audit-reference` — compiled
/// verbatim (never executed) so the published snippet cannot silently drift
/// from the real API.
#[allow(dead_code)]
fn website_snippet_exact() -> Result<(), Box<dyn std::error::Error>> {
    use zerodds_security::logging::LogLevel;
    use zerodds_security_logging::{FanOutLoggingPlugin, JsonLinesLoggingPlugin, StderrLoggingPlugin};

    let stderr_sink = StderrLoggingPlugin::with_level(LogLevel::Warning);
    let audit_sink = JsonLinesLoggingPlugin::open("/var/log/zerodds/audit.ndjson", LogLevel::Notice)?;

    let fanout = FanOutLoggingPlugin::new()
        .with(stderr_sink)
        .with(audit_sink);

    let security_bundle = SecurityBundle::builder()
        .logging_plugin(Box::new(fanout))
        // ... auth, access, crypto Plugins ...
        .build();

    let participant = DomainParticipantFactory::create(0)
        .with_security(security_bundle)
        .build()?;

    let _ = participant;
    Ok(())
}
