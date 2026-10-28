# crate-transport-tsn

The website's docs/transport-tsn.html usage snippet, made runnable.

The page spelled the type `IEEE802VlanTag`. No release ever carried that
name: it is `Ieee802VlanTag` at the v1.0.0-rc.6 tag and in the current tree,
which is what this demo uses. IEEE 802.1Q-2018 clause 9.6: TPID 0x8100,
PCP 6, DEI clear, VID 100.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-transport-tsn`.

Output of the run this README was written from:

```
transport-tsn: 802.1Q tag on the wire = [81, 00, c0, 64], roundtripped pcp=6 vid=100
```

## Reference

- Explained on the website: [TSN transport](https://zerodds.org/docs/transport-tsn.html)
- This example in the repository: [zero-dds-snippets/crate-transport-tsn](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-transport-tsn)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
