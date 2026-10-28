# crate-xml

The website's docs/xml.html usage snippet, made runnable. The snippet has
two calls and names no document; this supplies a minimal DDS-XML 1.0
building-block document for both of them.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-xml`.

Output of the run this README was written from:

```
xml: parse_dds_xml -> DdsXml { qos_libraries: [QosLibrary { name: "demo_lib", profiles: [QosProfile { name: "reliable", base_name: None, topic_filter: None, datawriter_qos: Some(EntityQos { durability: None, durability_service: None, presentation: None, deadline: None, latency_budget: None, ownership: None, ownership_strength: None, liveliness: None, time_based_filter: None, partition: None, reliability: Some(ReliabilityQosPolicy { kind: Reliable, max_blocking_time: Duration { seconds: 0, fraction: 429496729 } }), transport_priority: None, lifespan: None, destination_order: None, history: None, resource_limits: None, entity_factory: None, writer_data_lifecycle: None, reader_data_lifecycle: None, user_data: None, topic_data: None, group_data: None }), datareader_qos: None, topic_qos: None, publisher_qos: None, subscriber_qos: None, domainparticipant_qos: None }] }], domain_libraries: [], participant_libraries: [], application_libraries: [], type_libraries: [] }
xml: parse_xml_tree root element = "dds"
```

## Reference

- Explained on the website: [DDS-XML parser](https://zerodds.org/docs/xml.html)
- This example in the repository: [zero-dds-snippets/crate-xml](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-xml)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
