// Verbatim from documentation/03-configuration/security.md ("Code: wiring
// up the gate"). Reads the page's own "Governance XML" example
// (`governance.xml`, copied verbatim from the same page) and wires a
// `SharedSecurityGate` into a real `DcpsRuntime` — if this stops compiling
// or the runtime fails to start, the doc's security-wiring API drifted
// from the codebase.
//
// Needs the `security` feature on `zerodds-dcps` (`RuntimeConfig.security`
// only exists behind `#[cfg(feature = "security")]`) — see this crate's
// `Cargo.toml`.
use std::sync::Arc;

use zerodds_dcps::runtime::{DcpsRuntime, RuntimeConfig};
use zerodds_rtps::wire_types::GuidPrefix;
use zerodds_security_crypto::AesGcmCryptoPlugin;
use zerodds_security_permissions::parse_governance_xml;
use zerodds_security_runtime::SharedSecurityGate;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let governance = std::fs::read_to_string("governance.xml")?;
    let gov = parse_governance_xml(&governance)?;
    let crypto = Box::new(AesGcmCryptoPlugin::new());

    let gate = Arc::new(SharedSecurityGate::new(0, gov, crypto));

    let domain_id = 0; // matches <domains><id>0</id></domains> in governance.xml
    let prefix = GuidPrefix::from_bytes([0x0c; 12]);

    // DDS-Security 1.2 §9.3.3: under an active security profile the wire
    // GUID prefix is the one derived from the participant's identity
    // certificate, not a random candidate. `RuntimeConfig::start` refuses a
    // config that sets `security` and leaves `security_guid_prefix` unset —
    // it would come up with a prefix every peer rejects at handshake time,
    // and say nothing about it.
    //
    // A deployment gets both from its PKI in one step:
    //
    //     RuntimeConfig::default().with_security_bundle(&bundle)
    //
    // where `bundle` comes from the participant's `dds.sec.*` properties.
    // This example wires the governance gate alone, with no identity
    // certificate behind it, so it states the same prefix on both sides
    // itself.
    let cfg = RuntimeConfig {
        security: Some(gate),
        security_guid_prefix: Some(prefix),
        ..Default::default()
    };

    let rt = DcpsRuntime::start(domain_id, prefix, cfg)?;
    println!(
        "security-governance-gate: DcpsRuntime started on domain {domain_id} with the SharedSecurityGate wired"
    );
    rt.shutdown();
    Ok(())
}
