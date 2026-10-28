// The website's docs/websocket-bridge.html usage snippet, made runnable.
// RFC 6455 section 1.3 carries this exact nonce/accept pair as its own example.
use zerodds_websocket_bridge::compute_accept;

fn main() {
    let accept = compute_accept("dGhlIHNhbXBsZSBub25jZQ==");
    assert_eq!(accept, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
    println!("websocket-bridge: Sec-WebSocket-Accept = {accept}");
}
