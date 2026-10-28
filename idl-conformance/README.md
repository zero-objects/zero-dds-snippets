# IDL Conformance — Real-DCPS Roundtrip Examples

One standalone, runnable example per ZeroDDS language binding (PSM). Each
generates its topic type from IDL with `zerodds-idlc` on every run — nothing
generated is committed — writes fully-populated samples of a combined-feature
type through that binding's DDS API and reads them back, comparing every field.

| Example | Type | How to run |
|---|---|---|
| [rust](rust/) | `combo::Telemetry` | `cd rust && cargo run` |
| [cpp](cpp/) | `combo::Telemetry` | `cd cpp && ./build.sh` |
| [python](python/) | `combo::Telemetry` | `cd python && ./run.sh` |
| [java](java/) | `combo::Telemetry` | `cd java && ./build-and-run.sh` |
| [typescript](typescript/) | `combo::Telemetry` | `cd typescript && ./run.sh` |
| [csharp](csharp/) | `confcombo::Telemetry` | `cd csharp && ./run.sh` |
| [c](c/) | `conf::SensorReading` | see its README |

Each script builds `zerodds-idlc` from the ZeroDDS checkout this directory sits
in, or from `$ZERODDS_SRC`; each sub-directory's README names what else it
needs (the language toolchain, the built binding).

Beside the language examples:

* [`_interop/`](_interop/) — the wire conformance gate. `verify.sh` generates the
  codec of `features.idl` (eleven features) and `combo.idl` in all seven
  languages and requires every one to encode the canonical samples byte for
  byte as the Rust column's goldens, and to decode the goldens back
  ([`_interop/CANONICAL.md`](_interop/CANONICAL.md)).
* [`_qos/`](_qos/) — QoS-policy and keyed-lifecycle behaviour, observed per
  binding: RELIABILITY, DURABILITY, HISTORY, DEADLINE, LIVELINESS, OWNERSHIP,
  PARTITION, a ContentFilteredTopic and the instance lifecycle.
* [`proofs/`](proofs/) — ZeroDDS's bytes against recorded vendor output
  (Cyclone DDS, RTI Connext, Fast DDS, OpenDDS, open62541): KeyHash, the type
  system, TypeObjects, endianness × XCDR version, a structural feature, OPC UA
  UADP, RTPS framing and DDS-Security with XTypes.

`ci/examples.manifest` at the repository root lists every one of them with the
line its output must contain; `ci/run-examples.sh` runs them.

## Reference

- No page on the website explains this example yet; this README is the whole of it.
- This example in the repository: [zero-dds-snippets/idl-conformance](https://github.com/zero-objects/zero-dds-snippets/tree/main/idl-conformance)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
