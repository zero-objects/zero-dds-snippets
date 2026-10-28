# DDS QoS + keyed-lifecycle behavioral conformance — C# binding (real DCPS)

A single runnable harness (`Program.cs`) that drives the ZeroDDS C# binding over
a **real DomainParticipant + DataWriter/DataReader** on the keyed topic
`qos::Reading` and OBSERVES the effect of each QoS / lifecycle policy — not just
that a setter did not throw.

## Build & run

```sh
./run.sh
```

`run.sh` builds `zerodds-idlc` in the ZeroDDS checkout this directory sits in
(or `$ZERODDS_SRC`), generates `generated/` from `reading.idl`
(`zerodds-idlc generate --lang 'c#'`, not committed) and runs the project
against the native library in the checkout's `target/debug`.

Each check prints `VERIFIED` (behavior observed), `PARTIAL` or `UNSUPPORTED`.
Only `VERIFIED` counts: the program exits non-zero unless every check is, and
ends with `ALL 14 QOS CHECKS VERIFIED`.

## Checks

| # | Policy | What is observed |
|---|--------|------------------|
| 1 | RELIABILITY | RELIABLE delivers all 20 samples in order; BEST_EFFORT is accepted and delivers |
| 2 | DURABILITY | a late TRANSIENT_LOCAL reader receives the retained sample; a late VOLATILE reader nothing |
| 3 | HISTORY | KEEP_LAST(3) retains exactly the last three for a late joiner; KEEP_ALL retains all |
| 4 | DEADLINE | requested-deadline-missed fires after missed periods |
| 5 | LIVELINESS | AUTOMATIC lease: the reader goes not_alive after the writer stops |
| 6 | OWNERSHIP (EXCLUSIVE) | only the higher-strength writer's samples arrive |
| 7 | PARTITION | mismatched partitions are isolated, matching ones communicate |
| 8 | CONTENT-FILTERED-TOPIC | `Seq > 4` delivers exactly the matching samples |
| 9 | KEYED LIFECYCLE | two keys arrive ALIVE with distinct instance handles; dispose delivers NOT_ALIVE_DISPOSED; unregister (autodispose off) delivers NOT_ALIVE_NO_WRITERS |

## Files

| File | Role |
|------|------|
| `reading.idl` | keyed topic type `qos::Reading` (`@key long id; long seq; double value`) |
| `Program.cs` | the behavioral harness |
| `qos-csharp.csproj` | references the local binding; copies the native library to the output |
| `run.sh` | generates the type, builds and runs |
