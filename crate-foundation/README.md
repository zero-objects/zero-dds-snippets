# crate-foundation

The website's docs/foundation.html usage snippet, made runnable.
The two digests carry their specifications' own test vectors: RFC 3720
Appendix B.4 for CRC-32C over "123456789", RFC 1321 Appendix A.5 for MD5
over the empty string.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-foundation`.

Output of the run this README was written from:

```
foundation: crc32c("123456789") = 0xe3069283
foundation: md5("") = d41d8cd98f00b204e9800998ecf8427e
foundation: PoolBuffer<256> holds "hello"
```

## Reference

- Explained on the website: [Foundation](https://zerodds.org/docs/foundation.html)
- This example in the repository: [zero-dds-snippets/crate-foundation](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-foundation)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
