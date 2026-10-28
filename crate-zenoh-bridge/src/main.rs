// The website's docs/zenoh-bridge.html usage snippet, made runnable — and
// corrected. The snippet as published does not compile against any release:
//
//  1. `TopicMap::add` takes one `TopicMapEntry`, not a (topic, key) pair —
//     the same at the v1.0.0-rc.6 tag and in the current tree. `TopicMapEntry`
//     was not re-exported from the crate root either, so `add` took a type no
//     caller could name; the `pub use` was added while writing this.
//  2. `key_expr_for_topic` takes (scope, partition, topic) — the scope as
//     `Option<&str>` — and returns a `Result`: a segment holding a
//     Zenoh-reserved character (`* ? # $ /`) is refused, not rewritten.
//  3. A '.' in a DDS topic name is no reserved character and stays a '.':
//     `key_expr_for_topic(Some("fleet"), "", "VehicleTracking.TrackUpdate")`
//     is "fleet/VehicleTracking.TrackUpdate".
use zerodds_qos::{DurabilityKind, ReliabilityKind};
use zerodds_zenoh_bridge::{key_expr_for_topic, TopicMap, TopicMapEntry};

fn main() {
    let mut map = TopicMap::new();
    map.add(TopicMapEntry {
        topic: "VehicleTracking.TrackUpdate".into(),
        type_name: "VehicleTracking::TrackUpdate".into(),
        key_expr: "fleet/tracking".into(),
        reliability: ReliabilityKind::Reliable,
        durability: DurabilityKind::Volatile,
        dds_to_zenoh: true,
        zenoh_to_dds: false,
        keyed: false,
    });
    let entry = map
        .by_topic("VehicleTracking.TrackUpdate")
        .expect("the entry just added");
    assert_eq!(entry.key_expr, "fleet/tracking");

    // Without a partition, and with one.
    let key = key_expr_for_topic(Some("fleet"), "", "VehicleTracking.TrackUpdate")
        .expect("a valid key expression");
    assert_eq!(key, "fleet/VehicleTracking.TrackUpdate");
    assert_eq!(
        key_expr_for_topic(Some("dds"), "robot1", "Sensor").expect("a valid key expression"),
        "dds/robot1/Sensor"
    );
    // A Zenoh wildcard in a DDS topic name is refused.
    assert!(key_expr_for_topic(Some("dds"), "", "Foo*Bar").is_err());

    println!("zenoh-bridge: mapped topic -> {}", entry.key_expr);
    println!("zenoh-bridge: key expression = {key}");
}
