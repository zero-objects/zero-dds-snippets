# crate-xml-wire

The website's docs/zerodds-xml-wire.html usage snippet, made runnable.
DDS-XML 1.0 section 6.4: a sample encoded as an XML element per field and
decoded back to the same map.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-xml-wire`.

Output of the run this README was written from:

```
xml-wire: <Temperature><celsius xsi:type="xs:double">21.5</celsius></Temperature>
xml-wire: decoded type Temperature, 1 field(s), roundtripped
```

## Reference

- Explained on the website: [DDS-XML wire codec](https://zerodds.org/docs/zerodds-xml-wire.html)
- This example in the repository: [zero-dds-snippets/crate-xml-wire](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-xml-wire)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
