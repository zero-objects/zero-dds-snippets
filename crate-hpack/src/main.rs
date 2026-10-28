// The website's docs/hpack.html usage snippet, made runnable.
// RFC 7541: three header fields survive encode -> decode unchanged.
use zerodds_hpack::{Decoder, Encoder, HeaderField};

fn main() {
    let mut encoder = Encoder::new();
    let mut decoder = Decoder::new();

    let headers = vec![
        HeaderField { name: ":method".into(), value: "GET".into() },
        HeaderField { name: ":scheme".into(), value: "https".into() },
        HeaderField { name: "custom-key".into(), value: "custom-value".into() },
    ];

    let wire = encoder.encode(&headers);
    let decoded = decoder.decode(&wire).expect("roundtrip");
    assert_eq!(decoded, headers);
    println!("hpack: {} header fields in {} bytes, roundtripped", headers.len(), wire.len());
}
