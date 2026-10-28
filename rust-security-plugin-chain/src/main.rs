//! Demo for the `topics/security-plugin-chain/` snippets: the two Rust QoS
//! recipes published in the "Security Plugin-Chain Cookbook" — recipe 1
//! (PKI trust anchor: `participant_qos` with the `dds.sec.auth.*`
//! identity/cert/key properties) and recipe 5 (multi-sink audit logging:
//! `qos` with the `dds.sec.log.*` fan-out properties).
//!
//! Both snippets are reproduced verbatim from the website (with the
//! `&quot;` HTML entities unescaped to plain `"`). `main` builds both QoS
//! values, then wires the PKI one through a real participant.
//!
//! ## What the three auth properties are not enough for
//!
//! Recipe 1 names the Authentication plugin's three files (DDS-Security
//! 1.2 §9.3.1, Table 44). A participant that asks for security also needs
//! the Access Control plugin's three (§9.4.1.2, Table 46) —
//! `permissions_ca`, `governance`, `permissions`. Without them
//! `create_participant` refuses:
//!
//!     the participant QoS asks for DDS-Security and the property
//!     'dds.sec.access.permissions_ca' is not set
//!
//! That refusal is the point. The alternative — coming up in plaintext
//! because half the property list was missing — is a participant nobody
//! was told is unsecured. So this example states all six, and takes the
//! directory holding them from `ZERODDS_PKI`.

use std::path::Path;

use zerodds_dcps::qos::DomainParticipantQos;
use zerodds_dcps::DomainParticipantFactory;
use zerodds_qos::PropertyQosPolicy;

/// The six files DDS-Security 1.2 names, as (property, file name).
const MATERIAL: [(&str, &str); 6] = [
    // §9.3.1 Table 44 — Authentication.
    ("dds.sec.auth.identity_ca", "ca-cert.pem"),
    ("dds.sec.auth.identity_certificate", "part-cert.pem"),
    ("dds.sec.auth.private_key", "part-key.pem"),
    // §9.4.1.2 Table 46 — Access Control.
    ("dds.sec.access.permissions_ca", "permissions-ca-cert.pem"),
    ("dds.sec.access.governance", "governance.p7s"),
    ("dds.sec.access.permissions", "permissions.p7s"),
];

/// Recipe 1 — PKI trust anchor (website §recipe-pki-en), plus the Access
/// Control half a live participant needs, rooted at `dir`.
fn pki_qos(dir: &Path) -> DomainParticipantQos {
    let mut property = PropertyQosPolicy::new();
    for (name, file) in MATERIAL {
        property = property.with(name, &format!("file:{}", dir.join(file).display()));
    }
    DomainParticipantQos {
        property,
        ..Default::default()
    }
}

/// Recipe 5 — multi-sink audit logging (website §recipe-log-en).
fn audit_log_qos() -> DomainParticipantQos {
    // DDS-Security spec-style logger wireup: the participant carries
    // dds.sec.log.* properties; the runtime builds a fan-out logger from
    // them on create.
    DomainParticipantQos {
        property: PropertyQosPolicy::new()
            .with("dds.sec.log.plugin", "stderr,jsonl") // fan out to both
            .with("dds.sec.log.level", "Notice") // min level
            .with("dds.sec.log.jsonl.path", "/var/log/zerodds-audit.jsonl"),
        ..Default::default()
    }
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let log_qos = audit_log_qos();
    println!(
        "audit-log QoS built — log.plugin = {:?}",
        log_qos.property.get("dds.sec.log.plugin")
    );

    // The PKI half needs real files. Say which are missing rather than
    // building a participant that cannot be secured.
    let dir = match std::env::var_os("ZERODDS_PKI") {
        Some(d) => std::path::PathBuf::from(d),
        None => {
            eprintln!(
                "ZERODDS_PKI is not set. Point it at a directory holding the six \
                 files DDS-Security 1.2 §9.3.1 and §9.4.1.2 name: {}.",
                MATERIAL
                    .iter()
                    .map(|(_, f)| *f)
                    .collect::<Vec<_>>()
                    .join(", ")
            );
            std::process::exit(2);
        }
    };
    let missing: Vec<&str> = MATERIAL
        .iter()
        .filter(|(_, f)| !dir.join(f).exists())
        .map(|(_, f)| *f)
        .collect();
    if !missing.is_empty() {
        eprintln!(
            "ZERODDS_PKI={} is missing {}.",
            dir.display(),
            missing.join(", ")
        );
        std::process::exit(2);
    }

    let participant_qos = pki_qos(&dir);
    println!(
        "PKI QoS built — identity_ca = {:?}",
        participant_qos.property.get("dds.sec.auth.identity_ca")
    );

    // Wire the PKI QoS through a real participant via the fluent factory chain.
    let participant = DomainParticipantFactory::create(0)
        .with_qos(participant_qos)
        .build()?;

    println!(
        "participant up on domain {} — all six dds.sec.* files accepted",
        participant.domain_id()
    );

    DomainParticipantFactory::instance().delete_participant(&participant)?;
    Ok(())
}
