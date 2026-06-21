# IDL conformance — C# (XCDR2) combined-feature DCPS round-trip

A single **runnable** example that drives the ZeroDDS C# language binding
end-to-end over a **real DCPS participant + pub/sub loopback** (not just an
in-memory encode/decode): it builds one fully-populated, typed IDL sample,
publishes it with a typed `DataWriter`, takes it back on a same-participant
`DataReader`, and asserts field-by-field that the data survived the wire.

The IDL type (`conformance_combo.idl`) deliberately combines as many IDL
features as the C# stack can actually round-trip today, in one `@key`'d topic.
After the 2026-06 codegen/runtime fix pass the previously-blocked aggregate
features (nested struct, `sequence<struct>`, `map`, `union`, fixed array,
`typedef`-to-primitive, nested `sequence<sequence<…>>`) are now back in the
combo and verified over the wire.

## IDL features exercised (all asserted equal after the wire round-trip)

| Feature                         | IDL                          | Status |
|---------------------------------|------------------------------|--------|
| `@key` integral member          | `@key long unitId`           | ✅ |
| `@key` bounded string           | `@key string<32> region`     | ✅ |
| enum member                     | `Mode mode`                  | ✅ |
| double                          | `double batteryVolts`        | ✅ |
| `typedef`-to-primitive member   | `CurrentInAmps batteryCurrent` (`typedef double`) | ✅ |
| nested struct member            | `Position location`          | ✅ |
| `sequence<struct>`              | `sequence<Sample> history`   | ✅ |
| unbounded `sequence<long>`      | `sequence<long> samples`     | ✅ |
| bounded `sequence<long, N>`     | `sequence<long, 8> recentCodes` | ✅ |
| `sequence<string>`              | `sequence<string> tags`      | ✅ |
| nested `sequence<sequence<long>>` | `sequence<sequence<long>> matrix` | ✅ |
| `map<string, long>`             | `map<string, long> counters` | ✅ |
| `union` member (switch on enum) | `Reading reading`            | ✅ |
| fixed array                     | `long window[4]`             | ✅ |
| `@optional` (value **present**) | `@optional double calibration` | ✅ |
| `@appendable` extensibility (XCDR2 DHEADER) | struct annotation | ✅ |

The standalone `union` (`confcombo::Reading switch(Mode)`) also gets its own
`IDdsTopicType<>` `TypeSupport` with working encode/decode, in addition to being
embedded in `Telemetry`.

## Build & run

From this directory:

```sh
# 1. (Re)generate the C# type from the IDL with the pre-built compiler.
#    </dev/null is REQUIRED — the CLI reads stdin and otherwise hangs.
../../../target/debug/zerodds-idlc generate conformance_combo.idl --csharp -o . </dev/null

# 2. Build + run. The csproj references the local binding and copies the
#    already-built native libzerodds.dylib next to the managed assembly
#    (macOS SIP strips DYLD_LIBRARY_PATH from the dotnet host, so an env var
#    will NOT work — the copy is the reliable path).
dotnet run
```

Expected output ends with:

```
  OK   reading (union) sent=MODE_ACTIVE:99,5 got=MODE_ACTIVE:99,5
  OK   window (array[4]) sent=11,22,33,44 got=11,22,33,44
  OK   calibration    sent=0,0078125 got=0,0078125
  SKIP calibration=null   sent=null got=0 (known: absent @optional decodes to value-type default — see README)
ROUNDTRIP PASS — all supported fields survived the DCPS wire.
```

### Files

| File                    | Role |
|-------------------------|------|
| `conformance_combo.idl` | the combined-feature topic type |
| `conformance_combo.cs`  | verbatim `zerodds-idlc … --csharp` output (regenerate, do not hand-edit) |
| `OmgTypesShim.cs`       | minimal `Omg.Types` + `ZeroDDS.Cdr` attribute markers the generated code references but the binding does not yet ship (see note) |
| `Program.cs`            | the DCPS pub→sub round-trip + equality assertions |
| `csharp.csproj`         | references the local binding; copies the native dylib to output |

> **Note on the shim.** The generated `--csharp` output references
> `Omg.Types.ITopicType<T>`, `Omg.Types.ISequence<T>`,
> `Omg.Types.IBoundedSequence<T>`, `Omg.Types.SequenceList<T>` and the
> `[Key]` / `[Optional]` / `[Extensibility]` attributes (`ZeroDDS.Cdr`). Those
> support types are not shipped inside the `ZeroDDS` binding under test, so this
> example provides minimal markers for them — exactly the same pattern the
> `csharp-typed` website example uses for `ITopicType<T>`. The shim is plumbing
> only; the actual XCDR2 codec under test lives in the binding's
> `ZeroDDS.Cdr.Xcdr2Writer` / `Xcdr2Reader` and the generated `TypeSupport`.

## Known limitations (real, reproduced codegen bug)

The original goal was the full combo in
`tools/idlc/tests/conformance/fixtures/20_mixed_combo.idl` (enum + nested struct
+ sequence-of-struct + bounded string + union + map + optional + array + typedef
+ `@key`). After the 2026-06 fix pass, **all of those aggregate features now
round-trip** and are asserted equal in this example. One narrow gap remains:

1. **`@optional` *absent* loses null fidelity** — *codegen regression.* An
   `@optional double` whose value is **present** round-trips correctly (asserted
   above). But when the value is **absent** (`null`), the generated
   `TelemetryTypeSupport.DecodeFrom` declares the decode temp as a non-nullable
   `double __mNN = default!;` and only assigns inside `if (__present != 0)`. So
   an absent optional is materialised as the value-type default `0.0` instead of
   `null` (the present-bit and the missing payload *do* travel the wire
   correctly — only the C# null is lost on the way back into the `double?`
   field). Fix is in `crates/idl-csharp/src/typesupport.rs`: type the optional
   decode temp as the *nullable* form (`double?` / `T?`) so an absent member
   decodes to `null`. The program runs this case and reports it as `SKIP` rather
   than faking a pass.

Everything else in `conformance_combo.idl` — primitives, enum, typedef,
nested struct, `sequence<struct>`, bounded string, bounded/unbounded
`sequence<primitive>`, `sequence<string>`, nested `sequence<sequence<…>>`,
`map`, `union`, fixed array, `@optional` (present), `@key`, `@appendable` —
**does** round-trip cleanly end-to-end, which this example proves.

> Note: `zerodds-idlc … --csharp` still prints `warning: TypeObject emission
> skipped — UnsupportedTypeSpec("map (inline IDL map)")`. That warning is about
> the optional XTypes 1.3 *TypeObject metadata* blob for the map member, not the
> wire codec — the map `TypeSupport` (encode/decode) is fully generated and the
> map round-trips, as the `counters (map)` assertion shows.
