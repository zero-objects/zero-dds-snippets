// Demo for the website page:
//   website/topics/audit-logging/index.html — "4 · Syslog → SIEM" recipe.
//
// The page publishes the SyslogLoggingPlugin::connect(...) call reproduced
// verbatim below: a SocketAddr target, the app name, the hostname, and the
// minimum level. connect() binds a local ephemeral UDP socket (UDP is
// connectionless), so it succeeds even with no rsyslog listening — nothing has
// to exist at :514 for this to run. The sink is then wired into a
// SecurityBundle the same way rust-audit-securitybundle-realpart wires
// StderrLoggingPlugin.
use zerodds_dcps::qos::DomainParticipantQos;
use zerodds_dcps::runtime::RuntimeConfig;
use zerodds_dcps::DomainParticipantFactory;
use zerodds_security::logging::LogLevel;
use zerodds_security_logging::SyslogLoggingPlugin;
use zerodds_security_runtime::SecurityBundle;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Verbatim website snippet:
    let sink = SyslogLoggingPlugin::connect(
        "127.0.0.1:514".parse()?, // Lokaler rsyslog (SocketAddr)
        "zerodds",                // App-Name im Syslog-Stream
        "edge-01",                // Hostname im Syslog-Stream
        LogLevel::Notice,
    )?;

    // Wire it into a SecurityBundle, mirroring the *-realpart demo.
    let security_bundle = SecurityBundle::builder()
        .logging_plugin(Box::new(sink))
        // ... auth, access, crypto plugins ...
        .build();

    assert!(security_bundle.has_logging());

    // Real participant wiring (RuntimeConfig + create_participant_with_config),
    // exactly as in rust-audit-securitybundle-realpart.
    let cfg = RuntimeConfig::default().with_security_bundle(&security_bundle);
    let factory = DomainParticipantFactory::instance();
    let participant =
        factory.create_participant_with_config(0, DomainParticipantQos::default(), cfg)?;

    let _ = participant;
    println!("syslog logging plugin wired -> 127.0.0.1:514 (tag \"zerodds\", host \"edge-01\", LogLevel::Notice)");
    Ok(())
}
