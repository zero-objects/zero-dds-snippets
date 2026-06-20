//! Demo for the `website/topics/security-plugin-chain/index.html` snippets:
//! the two Rust QoS recipes published in the "Security Plugin-Chain Cookbook"
//! — recipe 1 (PKI trust anchor: `participant_qos` with the `dds.sec.auth.*`
//! identity/cert/key properties) and recipe 5 (multi-sink audit logging:
//! `qos` with the `dds.sec.log.*` fan-out properties).
//!
//! Both snippets are reproduced VERBATIM from the website (with the `&quot;`
//! HTML entities unescaped to plain `"`). `main` builds both QoS values and
//! wires the PKI one through a real participant via
//! `DomainParticipantFactory::create(0).with_qos(..).build()`.

use zerodds_dcps::qos::DomainParticipantQos;
use zerodds_dcps::DomainParticipantFactory;
use zerodds_qos::PropertyQosPolicy;

/// Recipe 1 — PKI trust anchor (website §recipe-pki-en).
fn pki_qos() -> DomainParticipantQos {
    let participant_qos = DomainParticipantQos {
        property: PropertyQosPolicy::new()
            .with("dds.sec.auth.identity_ca", "file:/etc/zerodds/ca-cert.pem")
            .with("dds.sec.auth.identity_certificate", "file:/etc/zerodds/part-cert.pem")
            .with("dds.sec.auth.private_key", "file:/etc/zerodds/part-key.pem"),
        ..Default::default()
    };
    participant_qos
}

/// Recipe 5 — multi-sink audit logging (website §recipe-log-en).
fn audit_log_qos() -> DomainParticipantQos {
    // DDS-Security spec-style logger wireup: the participant carries dds.sec.log.*
    // properties; the runtime builds a fan-out logger from them on create.
    let qos = DomainParticipantQos {
        property: PropertyQosPolicy::new()
            .with("dds.sec.log.plugin", "stderr,jsonl")                  // fan out to both
            .with("dds.sec.log.level", "Notice")                        // min level
            .with("dds.sec.log.jsonl.path", "/var/log/zerodds-audit.jsonl"),
        ..Default::default()
    };
    qos
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let participant_qos = pki_qos();
    let log_qos = audit_log_qos();

    println!(
        "PKI QoS built — identity_ca = {:?}",
        participant_qos.property.get("dds.sec.auth.identity_ca")
    );
    println!(
        "audit-log QoS built — log.plugin = {:?}",
        log_qos.property.get("dds.sec.log.plugin")
    );

    // Wire the PKI QoS through a real participant via the fluent factory chain.
    let participant = DomainParticipantFactory::create(0)
        .with_qos(participant_qos)
        .build()?;

    println!(
        "participant up on domain {} — dds.sec.auth.* properties carried on QoS",
        participant.domain_id()
    );

    DomainParticipantFactory::instance().delete_participant(&participant)?;
    Ok(())
}
