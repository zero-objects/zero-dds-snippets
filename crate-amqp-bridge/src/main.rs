// The website's docs/amqp-bridge.html usage snippet, made runnable.
// It claims an AMQP 1.0 `long` survives encode -> decode as AmqpValue::Long.
use zerodds_amqp_bridge::{decode_value, encode_long, AmqpValue};

fn main() {
    let buf = encode_long(42);
    let (v, _) = decode_value(&buf).expect("decode");
    assert_eq!(v, AmqpValue::Long(42));
    println!("amqp-bridge: {} bytes on the wire, decoded {:?}", buf.len(), v);
}
