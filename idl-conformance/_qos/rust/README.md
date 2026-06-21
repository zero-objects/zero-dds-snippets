# DDS QoS-policy + keyed-lifecycle behavioral conformance (Rust / DCPS)

Behavioral conformance harness for the ZeroDDS **Rust** binding over a real
`DomainParticipant` + `DataWriter`/`DataReader` (two participants per check, one
process, same domain, UDP-loopback runtime). Each check OBSERVES the actual QoS
effect, not just that a setter did not throw.

## Build & run

```sh
# (re)generate the keyed topic type
../../../../target/debug/zerodds-idlc --rust idl/reading.idl -o generated </dev/null
cargo run --bin qos_lifecycle
```

Topic type: `qos::Reading { @key long id; long seq; double value; }`.

## Results (observed)

| # | Policy                   | Status      | Evidence |
|---|--------------------------|-------------|----------|
| 1 | RELIABILITY              | verified    | RELIABLE delivered 50/50 in order; BEST_EFFORT matched + delivered |
| 2 | DURABILITY               | verified    | TRANSIENT_LOCAL late joiner got 3 retained; VOLATILE late joiner got nothing |
| 3 | HISTORY (KEEP_LAST)      | unsupported | KEEP_LAST(2) did NOT cap TL replay — late joiner saw all 5 (0..4) |
| 4 | DEADLINE                 | verified    | requested-deadline-missed 0→1 after writer fell silent |
| 5 | LIVELINESS               | verified    | AUTOMATIC: not_alive 0→1 (alive→not_alive) after writer dropped |
| 6 | OWNERSHIP (EXCLUSIVE)    | verified    | reader got 5 samples ALL from strength-10 writer, 0 from strength-1 |
| 7 | PARTITION                | unsupported | honored ONLY on DataWriter/Reader QoS; IGNORED on Publisher/Subscriber QoS |
| 8 | CONTENT-FILTERED-TOPIC   | verified    | `with_filter(value>10)` delivered 9 matching, dropped 0..=10 |
| 9 | KEYED LIFECYCLE          | unsupported | dispose()/unregister() return Ok but reader never sees NOT_ALIVE_* |

## Bugs found

### BUG-QOS-TYPEID (binding-api / dcps-runtime) — BLOCKS ALL MATCHING
The Rust backend emits an `EquivalenceHashComplete` `TYPE_IDENTIFIER` for a
normal `@final` struct. The runtime's writer↔reader type-consistency check
(`crates/dcps/src/runtime.rs:4785`) resolves both identifiers against a fresh
**empty** `TypeRegistry::new()`. In `crates/types/src/assignability.rs:198`,
`is_assignable` first calls `resolve_alias_chain`, which returns
`ResolveError::Unknown` for any hash not in the registry — so it returns
`Assignable::No` BEFORE the `w == r` identity short-circuit at line 227 can run.
Net effect: **a type cannot match an identical copy of itself**;
TYPE_CONSISTENCY_ENFORCEMENT (policy 23) is bumped and the endpoints never
match. There is no match arm for `(EquivalenceHashComplete, EquivalenceHashComplete)`
in `check_direct` at all (only `EquivalenceHashMinimal`). The baseline
`combo::Telemetry` example only works because its topic-level `TYPE_IDENTIFIER`
happens to be `None`, which skips the check entirely.

Workaround used so the QoS behaviors stay observable: `generated/reading.rs`
sets `TYPE_IDENTIFIER = None` (documented inline).

### BUG-QOS-PARTITION (binding-api) — PARTITION ignored on Publisher/Subscriber
`partitions_overlap` is correct and IS called in the match path, but the runtime
reads the partition list from the **DataWriterQos/DataReaderQos**
(`publisher.rs:316`, `subscriber.rs:271`: `qos.partition.names`). DDS-1.4 defines
PARTITION as a **Publisher/Subscriber** policy. Setting it on
`PublisherQos`/`SubscriberQos` (the spec-correct place) has NO effect — mismatched
partitions "X"/"Y" still communicate. Setting it on the writer/reader QoS works.

### BUG-QOS-LIFECYCLE (dcps-runtime) — dispose/unregister not delivered
With a fully matched writer+reader exchanging data, `writer.dispose(key)` and
`writer.unregister_instance(key)` return `Ok(())` (and update the writer's local
tracker), but the lifecycle DATA submessage never reaches the reader. The reader
keeps observing `ALIVE`; it never transitions to `NOT_ALIVE_DISPOSED` /
`NOT_ALIVE_NO_WRITERS`. The reader-side translation
(`runtime.rs delivered_to_user_sample` → `UserSample::Lifecycle` →
`subscriber.rs __push_lifecycle`) exists, so the gap is in delivery: the marker
is sent once via `user_unicast.send` (`runtime.rs:5630`) and never lands on the
matched reader's `rx` channel. (Secondary: a plain `write()` of a new key does
NOT make `lookup_instance` find that instance — only explicit `register_instance`
does.)

### BUG-QOS-HISTORY (dcps-runtime) — KEEP_LAST not capped on durability replay
A TRANSIENT_LOCAL writer with `History=KeepLast(2)` that writes 5 samples to one
instance replays ALL 5 to a late joiner; the per-instance depth-2 cap is not
applied on the durability replay path.
