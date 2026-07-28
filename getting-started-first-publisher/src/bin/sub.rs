// Verbatim from documentation/01-getting-started/first-publisher.md
// ("Subscriber" section, `src/bin/sub.rs`). Pairs with `pub.rs` — run both,
// see the README for the two-terminal invocation.
use std::time::Duration;

use zerodds_dcps::runtime::{DcpsRuntime, RuntimeConfig, UserReaderConfig, UserSample};
use zerodds_qos::{DeadlineQosPolicy, DurabilityKind, LivelinessQosPolicy, OwnershipKind};
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
        liveliness: LivelinessQosPolicy::default(),
        ownership: OwnershipKind::Shared,
        partition: vec![],
        user_data: vec![],
        topic_data: vec![],
        group_data: vec![],
        type_identifier: TypeIdentifier::Primitive(PrimitiveKind::UInt8),
        type_consistency: TypeConsistencyEnforcement::default(),
        data_representation_offer: None,
    })?;

    let deadline = std::time::Instant::now() + Duration::from_secs(15);
    while std::time::Instant::now() < deadline {
        match rx.recv_timeout(Duration::from_millis(500)) {
            Ok(UserSample::Alive { payload, .. }) => {
                println!("sub: got {}", String::from_utf8_lossy(payload.as_ref()));
            }

            Ok(UserSample::Lifecycle { key_hash, kind }) => {
                println!("lifecycle: {:?} {:?}", kind, key_hash);
            }

            Err(_) => {}
        }
    }
    rt.shutdown();
    Ok(())
}
