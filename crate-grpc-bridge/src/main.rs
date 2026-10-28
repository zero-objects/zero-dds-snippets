// The website's docs/grpc-bridge.html usage snippet, made runnable.
// The gRPC length-prefixed message framing: 1 compression byte + 4-byte
// big-endian length + payload.
use zerodds_grpc_bridge::{decode_message, encode_message};

fn main() {
    let msg = b"hello-grpc";
    let wire = encode_message(msg, false).expect("encode");
    let (flag, payload, consumed) = decode_message(&wire).expect("decode");
    assert_eq!(flag, 0);
    assert_eq!(payload, msg);
    assert_eq!(consumed, wire.len());
    println!("grpc-bridge: {} bytes framed, flag {}, payload {:?}", wire.len(), flag, core::str::from_utf8(&payload).unwrap());
}
