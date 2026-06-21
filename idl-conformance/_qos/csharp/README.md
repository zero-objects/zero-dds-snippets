# DDS QoS + keyed-lifecycle behavioral conformance — C# binding (real DCPS)

A single runnable harness (`Program.cs`) that drives the ZeroDDS C# binding over
a **real DomainParticipant + DataWriter/DataReader** on the keyed topic
`qos::Reading` and OBSERVES the effect of each QoS / lifecycle policy — not just
that a setter did not throw.

## Build & run

```sh
# 1. (Re)generate the C# type (</dev/null is REQUIRED — CLI reads stdin).
../../../../target/debug/zerodds-idlc generate reading.idl --csharp -o . </dev/null

# 2. Build + run.
dotnet run
```

Each line is printed as `VERIFIED` (behavior observed), `PARTIAL` (policy
accepted + QoS-matched but effect not independently observable) or `UNSUPPORTED`
(a real gap). `ran_ok` = the process completed with 0 hard failures.

## Observed results (this run)

| # | Policy | Verdict | What was observed |
|---|--------|---------|-------------------|
| 1 | RELIABILITY | verified | RELIABLE delivered all 20 samples in order; BEST_EFFORT accepted + delivered |
| 2 | DURABILITY (VOLATILE) | verified | late VOLATILE reader correctly got nothing |
| 2 | DURABILITY (TRANSIENT_LOCAL) | **unsupported** | late reader matches + live delivery works, but receives **0 historical samples** — writer-cache late-join replay is not performed on the C-FFI path |
| 3 | HISTORY (KEEP_LAST/KEEP_ALL) | partial | policy accepted + QoS-matched; cap is not independently observable because the only behavioral probe (TRANSIENT_LOCAL replay) is itself broken |
| 4 | DEADLINE | verified | requested-deadline-missed fired (TotalCount=5 after ~5 missed 300 ms periods) |
| 5 | LIVELINESS | verified | AUTOMATIC lease: liveliness dropped to not_alive after writer stopped (NotAlive=1) |
| 6 | OWNERSHIP (EXCLUSIVE) | **unsupported** | reader saw BOTH a strength-1 and a strength-10 writer; arbitration not applied |
| 7 | PARTITION | **unsupported** | mismatched partitions A/B still communicate; PartitionPolicy is dropped before native |
| 8 | CONTENT-FILTERED-TOPIC | **unsupported** | no CFT type and no query/filter method on `DataReader<T>` |
| 9 | KEYED LIFECYCLE (ALIVE) | verified | 2 key instances (Id=100,200) delivered, both ALIVE |
| 9 | KEYED LIFECYCLE (dispose/unregister) | **unsupported** | `DataWriter<T>` exposes no dispose-by-key / unregister-by-key API → NOT_ALIVE_DISPOSED / NOT_ALIVE_NO_WRITERS unreachable |

## Root-cause notes (where the gap lives)

* **PARTITION — binding-api.** `ZeroDDS/src/QosBridge.cs` hardcodes every
  `NativePartition { Names = IntPtr.Zero, NamesLen = 0 }`, so the configured
  partition names never cross the FFI. (The native `ZeroDdsPartitionQosPolicy`
  *does* carry names; the binding drops them. The C-FFI
  `UserWriterConfig.partition` is also hardcoded `Vec::new()`.)
* **CONTENT-FILTERED-TOPIC — binding-api.** The native lib ships
  `zerodds_dp_create_contentfilteredtopic` and `zerodds_dr_create_querycondition`,
  but `ZeroDDS/src/Native.cs` does not declare CFT, and `DataReader<T>.Take`
  applies no query filter. No public CFT type exists.
* **dispose/unregister lifecycle — binding-api.** `Native.DwDispose`
  (`zerodds_dw_dispose`) exists and the generated `ReadingTypeSupport.KeyHash`
  can produce the 16-byte key, but `DataWriter<T>` wraps neither dispose nor
  unregister, and `Native.cs` declares no `unregister_instance` /
  `register_instance` entry point. The disposed/unregistered instance states are
  therefore unreachable from the public C# surface.
* **OWNERSHIP EXCLUSIVE — dcps-runtime / C-FFI.** Exclusive-ownership
  arbitration (`passes_exclusive_ownership`) is implemented only in the typed
  Rust `DataReader<T>`; the C-FFI take path (`zerodds_dr_take` →
  `SampleArray::from_state`) passes every `UserSample::Alive` payload through
  with no arbitration, so every C-FFI binding (C#, C, C++, Python, …) bypasses it.
* **TRANSIENT_LOCAL late-join replay — dcps-runtime.** For TRANSIENT_LOCAL the
  writer cache is the history anchor, but no replay of that cache to a
  late-joining reader is performed on the in-process C-FFI path (and no
  DurabilityService backend is attached), so the late joiner gets 0 samples.
* **SampleInfo.InstanceHandle — dcps-runtime / C-FFI (minor).** The C-FFI take
  path hardcodes `instance_handle = 0` for every sample, so per-instance
  identity is not observable via the handle (the decoded `@key` still works).

## Files

| File | Role |
|------|------|
| `reading.idl` | keyed topic type `qos::Reading` (`@key long id; long seq; double value`) |
| `reading.cs` | verbatim `zerodds-idlc … --csharp` output (regenerate, do not hand-edit) |
| `OmgTypesShim.cs` | minimal `Omg.Types.ITopicType<T>` + `[Key]` markers the generated code references |
| `Program.cs` | the 9-policy behavioral harness |
| `qos-csharp.csproj` | references the local binding; copies the native dylib to output |
