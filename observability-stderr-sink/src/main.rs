// Verbatim from documentation/03-configuration/observability.md ("Wire it
// up", Layer 2: Sink-based events). Registers a writer and a reader so the
// StderrJsonSink actually emits the `user_writer.created` /
// `user_reader.created` lines the doc shows as sample output — if this
// stops compiling or stops printing JSON, the doc's API drifted from the
// codebase.
use std::sync::Arc;

use zerodds_dcps::runtime::{DcpsRuntime, RuntimeConfig, UserReaderConfig, UserWriterConfig};
use zerodds_foundation::observability::StderrJsonSink;
use zerodds_qos::{
    DeadlineQosPolicy, DestinationOrderQosPolicy, DurabilityKind, LatencyBudgetQosPolicy,
    LifespanQosPolicy, LivelinessQosPolicy, OwnershipKind, PresentationQosPolicy,
};
use zerodds_rtps::wire_types::GuidPrefix;
use zerodds_types::{PrimitiveKind, TypeIdentifier};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let domain_id = 9;
    let prefix = GuidPrefix::from_bytes([0x09; 12]);

    // Doc fence, verbatim:
    let cfg = RuntimeConfig {
        observability: Arc::new(StderrJsonSink::new()),
        ..Default::default()
    };
    let rt = DcpsRuntime::start(domain_id, prefix, cfg)?;

    // Trigger the `user_writer.created` / `user_reader.created` events the
    // doc's stderr-output block shows.
    let _writer_eid = rt.register_user_writer(UserWriterConfig {
        topic_name: "Telemetry".into(),
        type_name: "Robot::Pose".into(),
        reliable: true,
        durability: DurabilityKind::Volatile,
        durability_service: zerodds_qos::DurabilityServiceQosPolicy::default(),
        deadline: DeadlineQosPolicy::default(),
        latency_budget: LatencyBudgetQosPolicy::default(),
        destination_order: DestinationOrderQosPolicy::default(),
        presentation: PresentationQosPolicy::default(),
        lifespan: LifespanQosPolicy::default(),
        liveliness: LivelinessQosPolicy::default(),
        ownership: OwnershipKind::Shared,
        ownership_strength: 0,
        partition: vec![],
        user_data: vec![],
        topic_data: vec![],
        group_data: vec![],
        type_identifier: TypeIdentifier::Primitive(PrimitiveKind::UInt8),
        data_representation_offer: None,
        ..Default::default()
    })?;

    let (_reader_eid, _rx) = rt.register_user_reader(UserReaderConfig {
        topic_name: "Commands".into(),
        type_name: "Robot::Cmd".into(),
        type_identifier: TypeIdentifier::Primitive(PrimitiveKind::UInt8),
        ..Default::default()
    })?;

    println!("observability-stderr-sink: writer + reader created (see stderr for JSON events)");
    rt.shutdown();
    Ok(())
}
