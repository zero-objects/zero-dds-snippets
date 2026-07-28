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

    let cfg = RuntimeConfig {
        security: Some(gate),
        ..Default::default()
    };

    let domain_id = 0; // matches <domains><id>0</id></domains> in governance.xml
    let prefix = GuidPrefix::from_bytes([0x0c; 12]);
    let rt = DcpsRuntime::start(domain_id, prefix, cfg)?;
    println!(
        "security-governance-gate: DcpsRuntime started on domain {domain_id} with the SharedSecurityGate wired"
    );
    rt.shutdown();
    Ok(())
}
