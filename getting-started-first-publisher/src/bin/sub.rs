// Verbatim from documentation/01-getting-started/first-publisher.md
// ("Subscriber" section, `src/bin/sub.rs`). Pairs with `pub.rs` — run both,
// see the README for the two-terminal invocation.
use std::time::Duration;

use zerodds_dcps::runtime::{DcpsRuntime, RuntimeConfig, UserReaderConfig, UserSample};
use zerodds_qos::{
    DeadlineQosPolicy, DestinationOrderQosPolicy, DurabilityKind, LatencyBudgetQosPolicy,
    LivelinessQosPolicy, OwnershipKind, PresentationQosPolicy, ResourceLimitsQosPolicy,
};
use zerodds_rtps::wire_types::GuidPrefix;
use zerodds_types::{PrimitiveKind, TypeIdentifier, qos::TypeConsistencyEnforcement};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let domain_id = 1;
    let prefix = GuidPrefix::from_bytes([0x02; 12]);
    let rt = DcpsRuntime::start(domain_id, prefix, RuntimeConfig::default())?;

    let (_eid, rx) = rt.register_user_reader(UserReaderConfig {
        topic_name: "HelloTopic".into(),
        type_name: "RawBytes".into(),
        reliable: true,
        durability: DurabilityKind::Volatile,
        deadline: DeadlineQosPolicy::default(),
        // RxO-matched pairs, DDS 1.4 §2.2.3.10 and §2.2.3.18: a writer
        // offers and a reader requests, and a mismatch is a
        // non-match rather than a silent downgrade.
        latency_budget: LatencyBudgetQosPolicy::default(),
        destination_order: DestinationOrderQosPolicy::default(),
        liveliness: LivelinessQosPolicy::default(),
        ownership: OwnershipKind::Shared,
        partition: vec![],
        user_data: vec![],
        topic_data: vec![],
        group_data: vec![],
        type_identifier: TypeIdentifier::Primitive(PrimitiveKind::UInt8),
        // Publisher/Subscriber group scope, DDS 1.4 §2.2.3.6: INSTANCE,
        // no coherent or ordered access.
        presentation: PresentationQosPolicy::default(),
        type_consistency: TypeConsistencyEnforcement::default(),
        // RESOURCE_LIMITS, DDS 1.4 §2.2.3.19: how much this reader is
        // willing to hold. `max_samples` reaches the reliable receive
        // cache; LENGTH_UNLIMITED — the default — leaves the runtime's
        // own bound in place, because a cache needs one.
        resource_limits: ResourceLimitsQosPolicy::default(),
        data_representation_offer: None,
        ..Default::default()
    })?;

    let deadline = std::time::Instant::now() + Duration::from_secs(15);
    while std::time::Instant::now() < deadline {
        match rx.recv_timeout(Duration::from_millis(500)) {
            Ok(UserSample::Alive { payload, .. }) => {
                println!("sub: got {}", String::from_utf8_lossy(payload.as_ref()));
            }

            Ok(UserSample::Lifecycle { key_hash, kind, .. }) => {
                println!("lifecycle: {:?} {:?}", kind, key_hash);
            }

            // End-of-coherent-set, DDSI-RTPS 2.5 §8.7.6: the writer closed
            // the set that starts at `set_first_sn`. It carries no user
            // data, and a reader without `coherent_access` has nothing to
            // release — but the arm is written out rather than swept into a
            // `_ =>`, so a future variant is a compile error here and not a
            // sample this loop drops without saying so.
            // `set_first_sn` is `None` when the writer names no first
            // sequence number (Table 9.22: SEQUENCENUMBER_UNKNOWN).
            Ok(UserSample::CoherentSetEnd { set_first_sn, .. }) => match set_first_sn {
                Some(sn) => println!("coherent set starting at {sn} closed"),
                None => println!("coherent set closed"),
            },

            Err(_) => {}
        }
    }
    rt.shutdown();
    Ok(())
}
