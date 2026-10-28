// The website's docs/http2.html usage snippet, made runnable.
// RFC 9113 section 4.1: a 9-byte frame header plus an 8-byte PING payload.
use zerodds_http2::frame::DEFAULT_MAX_FRAME_SIZE;
use zerodds_http2::{decode_frame, encode_frame, Flags, FrameHeader, FrameType};

fn main() {
    let payload = [0u8; 8];
    let header = FrameHeader {
        length: 8,
        frame_type: FrameType::Ping,
        flags: Flags(0),
        stream_id: 0,
    };

    let mut buf = [0u8; 17];
    encode_frame(&header, &payload, &mut buf, DEFAULT_MAX_FRAME_SIZE).unwrap();
    let (decoded, consumed) = decode_frame(&buf, DEFAULT_MAX_FRAME_SIZE).unwrap();
    assert_eq!(consumed, 17);
    assert_eq!(decoded.header.frame_type, FrameType::Ping);
    println!("http2: {consumed} bytes consumed, frame type {:?}", decoded.header.frame_type);
}
