// The website's docs/foundation.html usage snippet, made runnable.
// The two digests carry their specifications' own test vectors: RFC 3720
// Appendix B.4 for CRC-32C over "123456789", RFC 1321 Appendix A.5 for MD5
// over the empty string.
use zerodds_foundation::{crc32c, md5, PoolBuffer};

fn main() {
    assert_eq!(crc32c(b"123456789"), 0xE306_9283);
    assert_eq!(
        md5(b""),
        [
            0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04, 0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8,
            0x42, 0x7e,
        ]
    );

    // Stack buffer (no allocations)
    let mut buf: PoolBuffer<256> = PoolBuffer::new();
    buf.extend_from_slice(b"hello").unwrap();
    assert_eq!(buf.as_slice(), b"hello");

    println!("foundation: crc32c(\"123456789\") = {:#010x}", crc32c(b"123456789"));
    println!("foundation: md5(\"\") = {}", md5(b"").iter().map(|b| format!("{b:02x}")).collect::<String>());
    println!("foundation: PoolBuffer<256> holds {:?}", core::str::from_utf8(buf.as_slice()).unwrap());
}
