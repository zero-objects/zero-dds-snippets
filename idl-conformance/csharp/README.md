# C# combined-feature typed DCPS round-trip

This example uses fresh output from the current `zerodds-idlc`, its production
`IDdsTopicType<T>` companion, the managed wire runtime, and a real native DDS
participant. It is not an in-memory bus or a codec-only test.

The IDL's qualified `confcombo::Mode` and `confcombo::Reading` member types are
intentional. They avoid case-insensitive IDL name collisions without renaming
members or changing their types.

## What must pass

Two independently constructed `Telemetry` samples traverse the native typed
writer/reader. Every field is compared after receipt, including nested structs,
sequences and nested sequences, map entries, the union's selected branch,
transparent primitive typedef, fixed array, and both key members.

The first sample has a present optional calibration and the union's active
double branch. The second has a different key, an absent calibration, and the
union's default fault-string branch. The selected branch is compared according
to its discriminator. Timeout, missing sample, wrong sample identity,
unexpected sample count, or an absent optional decoded as zero is a failure.
There is no `SKIP` path for optional absence. Entities are disposed on success
and on exceptions.

## Generate and run

With the native library built in the ZeroDDS checkout this directory sits in
(or `$ZERODDS_SRC`):

```sh
./run.sh
```

`run.sh` builds `zerodds-idlc`, generates `generated/` from
`conformance_combo.idl` (`zerodds-idlc generate --lang 'c#' --encoding xcdr2`),
and runs the project with `dotnet run -- --domain 177` (`DOMAIN` overrides it).

Use a domain isolated from other tests. Invalid arguments fail instead of
silently choosing another domain. For an isolated generated output or native
build directory, pass absolute paths through `-p:GeneratedDirectory=PATH` and
`-p:NativeLibraryDirectory=PATH` before `--`.

The project copies the native library beside the managed executable, including
on macOS where the host may strip dynamic-library environment variables. It
requires the current native typed-wire and keyed-write APIs. No legacy API fallback or shim is
used. Compilation treats warnings as errors.

## Source and generated artifacts

- `conformance_combo.idl` is the authoritative combined-feature type.
- `Program.cs` contains the sample construction and native round-trip assertions.
- `generated/conformance_combo.cs` contains fresh types and wire codecs.
- `generated/conformance_combo.TypeSupport.cs` contains production typed DDS
  support with generated TypeObject metadata and key serialization.
- `csharp.csproj` explicitly selects those three C# inputs and references the
  local DDS binding and shared wire runtime.

`generated/` is not committed. Do not hand-edit generated files or add a
compatibility shim to make this example pass. Unsupported metadata must
remain a visible registration failure, not an empty TypeObject.
