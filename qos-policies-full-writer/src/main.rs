// Verbatim from documentation/03-configuration/qos-policies.md ("Setting
// QoS in code"). The doc fence only constructs the `UserWriterConfig`; this
// companion additionally starts a real `DcpsRuntime` and registers the
// writer, proving the full QoS combination (TransientLocal + Exclusive
// ownership + Deadline/Lifespan/Liveliness + partition) is actually
// accepted at registration time, not just field-compatible.
use zerodds_dcps::runtime::{DcpsRuntime, RuntimeConfig, UserWriterConfig};
use zerodds_qos::*;
use zerodds_rtps::wire_types::GuidPrefix;
use zerodds_types::{PrimitiveKind, TypeIdentifier};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let domain_id = 10;
    let prefix = GuidPrefix::from_bytes([0x0a; 12]);
    let rt = DcpsRuntime::start(domain_id, prefix, RuntimeConfig::default())?;

    // Doc fence, verbatim:
    let cfg = UserWriterConfig {
        topic_name: "Telemetry".into(),
        type_name: "Robot::Pose".into(),
        reliable: true, // = Reliability::Reliable
        durability: DurabilityKind::TransientLocal,
        deadline: DeadlineQosPolicy {
            period: Duration::from_millis(50),
        },
        lifespan: LifespanQosPolicy {
            duration: Duration::from_secs(5),
        },
        liveliness: LivelinessQosPolicy {
            kind: LivelinessKind::Automatic,
            lease_duration: Duration::from_secs(2),
        },
        ownership: OwnershipKind::Exclusive,
        ownership_strength: 100,
        partition: vec!["sensor.*".into()],
        user_data: vec![],
        topic_data: vec![],
        group_data: vec![],
        type_identifier: TypeIdentifier::Primitive(PrimitiveKind::UInt8),
        data_representation_offer: None,
    };

    let writer_eid = rt.register_user_writer(cfg)?;
    println!("qos-policies-full-writer: registered {writer_eid:?}");

    rt.shutdown();
    Ok(())
}
