# crate-discovery

The website's docs/discovery.html usage snippet, made runnable.

The snippet names `participant_data` and `incoming` without defining them.
This builds a real ParticipantBuiltinTopicData, serializes the SPDP beacon
datagram the snippet would put on the multicast group, and feeds that same
datagram to SpdpReader — the "receive a peer's beacon" half of the snippet,
with the loop closed so the run has something to check.
DDSI-RTPS 2.5 section 8.5.3.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-discovery`, `zerodds-rtps`.

Output of the run this README was written from:

```
discovery: SPDP beacon is 172 bytes
discovery: parsed peer GUID Guid { prefix: GuidPrefix([10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10]), entity_id: EntityId { entity_key: [0, 0, 1], entity_kind: Participant } }
```

## Reference

- Explained on the website: [Discovery](https://zerodds.org/docs/discovery.html)
- This example in the repository: [zero-dds-snippets/crate-discovery](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-discovery)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
