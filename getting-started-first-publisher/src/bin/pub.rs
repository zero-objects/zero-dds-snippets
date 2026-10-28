// Verbatim from documentation/01-getting-started/first-publisher.md
// ("Publisher" section, `src/bin/pub.rs`). The runnable companion to the
// low-level DcpsRuntime pub/sub walkthrough — if this stops compiling, the
// doc's API drifted from the codebase.
use std::thread::sleep;
use std::time::Duration;

use zerodds_dcps::runtime::{DcpsRuntime, RuntimeConfig, UserWriterConfig};
use zerodds_qos::{
    DeadlineQosPolicy, DestinationOrderQosPolicy, DurabilityKind, LatencyBudgetQosPolicy,
    LifespanQosPolicy, LivelinessQosPolicy, OwnershipKind, PresentationQosPolicy,
};
use zerodds_rtps::wire_types::GuidPrefix;
use zerodds_types::{PrimitiveKind, TypeIdentifier};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let domain_id = 1;
    let prefix = GuidPrefix::from_bytes([0x01; 12]);
    let rt = DcpsRuntime::start(domain_id, prefix, RuntimeConfig::default())?;

    let writer_eid = rt.register_user_writer(UserWriterConfig {
        topic_name: "HelloTopic".into(),
        type_name: "RawBytes".into(),
        reliable: true,
        durability: DurabilityKind::Volatile,
        durability_service: zerodds_qos::DurabilityServiceQosPolicy::default(),
        deadline: DeadlineQosPolicy::default(),
        // RxO-matched pairs, DDS 1.4 §2.2.3.10 and §2.2.3.18: a writer
        // offers and a reader requests, and a mismatch is a
        // non-match rather than a silent downgrade.
        latency_budget: LatencyBudgetQosPolicy::default(),
        destination_order: DestinationOrderQosPolicy::default(),
        lifespan: LifespanQosPolicy::default(),
        liveliness: LivelinessQosPolicy::default(),
        ownership: OwnershipKind::Shared,
        ownership_strength: 0,
        partition: vec![],
        user_data: vec![],
        topic_data: vec![],
        group_data: vec![],
        type_identifier: TypeIdentifier::Primitive(PrimitiveKind::UInt8),
        // Publisher/Subscriber group scope, DDS 1.4 §2.2.3.6: INSTANCE,
        // no coherent or ordered access.
        presentation: PresentationQosPolicy::default(),
        data_representation_offer: None,
        ..Default::default()
    })?;

    for i in 0u32..10 {
        let payload = format!("hello {i}").into_bytes();
        rt.write_user_sample(writer_eid, payload)?;
        println!("pub: sent {i}");
        sleep(Duration::from_millis(500));
    }
    rt.shutdown();
    Ok(())
}
