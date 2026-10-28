// The website's docs/mqtt-bridge.html usage snippet, made runnable.
// MQTT 3.1.1 section 4.7.1: the single-level `+` and multi-level `#` wildcards.
use zerodds_mqtt_bridge::topic_matches;

fn main() {
    assert!(topic_matches("sensors/+/temp", "sensors/room1/temp"));
    assert!(topic_matches("sensors/#", "sensors/room1/temp/f"));
    println!("mqtt-bridge: both wildcard filters matched");
}
