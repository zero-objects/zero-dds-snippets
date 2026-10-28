# DDS QoS-policy + keyed-lifecycle behavioral conformance (Rust / DCPS)

Behavioral conformance harness for the ZeroDDS **Rust** binding over a real
`DomainParticipant` + `DataWriter`/`DataReader` (two participants per check, one
process, same domain, UDP-loopback runtime). Each check OBSERVES the actual QoS
effect, not just that a setter did not throw.

## Build & run

```sh
cargo run --quiet
```

`build.rs` generates the keyed topic type from `idl/reading.idl` with
`zerodds-build` on every build. The program exits non-zero unless every
check is VERIFIED and ends with `ALL 9 QOS CHECKS VERIFIED`.

Topic type: `qos::Reading { @key long id; long seq; double value; }`.

## Checks

| # | Policy                   | What is observed |
|---|--------------------------|------------------|
| 1 | RELIABILITY              | RELIABLE delivers 50/50 in order; BEST_EFFORT matches and delivers |
| 2 | DURABILITY               | a TRANSIENT_LOCAL late joiner receives the retained samples; a VOLATILE one nothing |
| 3 | HISTORY (KEEP_LAST)      | KEEP_LAST(2) caps the late joiner's replay to the two newest |
| 4 | DEADLINE                 | requested-deadline-missed increments after the writer falls silent |
| 5 | LIVELINESS               | AUTOMATIC: not_alive_count 0→1 after the writer stops |
| 6 | OWNERSHIP (EXCLUSIVE)    | all samples come from the strength-10 writer, none from strength-1 |
| 7 | PARTITION                | enforced on the Publisher/Subscriber and on the DataWriter/DataReader QoS |
| 8 | CONTENT-FILTERED-TOPIC   | `value > 10` delivers only the matching samples |
| 9 | KEYED LIFECYCLE          | ALIVE → NOT_ALIVE_DISPOSED → ALIVE on a new sample, NOT_ALIVE_NO_WRITERS on unregister |
