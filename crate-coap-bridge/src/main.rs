// The website's docs/coap-bridge.html usage snippet, made runnable.
// RFC 7252 header round-trip: a Confirmable GET keeps its code across the wire.
use zerodds_coap_bridge::{decode, encode, CoapCode, CoapMessage, MessageType};

fn main() {
    let msg = CoapMessage {
        version: 1,
        message_type: MessageType::Confirmable,
        token: Vec::new(),
        code: CoapCode::GET,
        message_id: 0xBEEF,
        options: Vec::new(),
        payload: Vec::new(),
    };

    let wire = encode(&msg).expect("encode");
    let decoded = decode(&wire).expect("decode");
    assert_eq!(decoded.code, CoapCode::GET);
    println!("coap-bridge: {} bytes on the wire, decoded code {:?}", wire.len(), decoded.code);
}
