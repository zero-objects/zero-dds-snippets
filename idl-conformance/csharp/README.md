# IDL conformance — C# (XCDR2) combined-feature DCPS round-trip

A single **runnable** example that drives the ZeroDDS C# language binding
end-to-end over a **real DCPS participant + pub/sub loopback** (not just an
in-memory encode/decode): it builds one fully-populated, typed IDL sample,
publishes it with a typed `DataWriter`, takes it back on a same-participant
`DataReader`, and asserts field-by-field that the data survived the wire.

The IDL type (`conformance_combo.idl`) deliberately combines as many IDL
features as the C# stack can actually round-trip today, in one `@key`'d topic.

## IDL features exercised (all asserted equal after the wire round-trip)

| Feature                         | IDL                          | Status |
|---------------------------------|------------------------------|--------|
| `@key` integral member          | `@key long unitId`           | ✅ |
| `@key` bounded string           | `@key string<32> region`     | ✅ |
| enum member                     | `Mode mode`                  | ✅ |
| double                          | `double batteryVolts`        | ✅ |
| unbounded `sequence<long>`      | `sequence<long> samples`     | ✅ |
| bounded `sequence<long, N>`     | `sequence<long, 8>`          | ✅ |
| `sequence<string>`              | `sequence<string> tags`      | ✅ |
| `@optional` (present **and** absent) | `@optional double calibration` | ✅ |
| `@appendable` extensibility (XCDR2 DHEADER) | struct annotation | ✅ |

The program publishes two samples: one with the optional **present**, one with
it **absent** (`null`), and asserts the optional round-trips as `null` (not a
defaulted `0.0`).

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
  OK   unitId         sent=42 got=42
  ...
  OK   calibration=null sent=null got=
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

## Known limitations (real, reproduced codegen/runtime bugs)

The original goal was the full combo in
`tools/idlc/tests/conformance/fixtures/20_mixed_combo.idl` (enum + nested struct
+ sequence-of-struct + bounded string + union + map + optional + array + typedef
+ `@key`). The following members had to be **excluded** because the C# backend
cannot round-trip them today. Each was reproduced with a minimal in-memory
encode→decode (the codec is self-inconsistent — no DDS transport involved):

1. **Nested struct member, and `sequence<struct>`** — *C# runtime bug, breaks the wire.*
   `Xcdr2Reader` is a `ref struct` (value type), but the generated decoder calls
   nested `XxxTypeSupport.Instance.DecodeFrom(r)` passing the reader **by value**.
   The nested decode advances a *copy* of the reader, so the parent's cursor never
   moves past the nested object. The next field is then read from the wrong
   offset and decode desyncs (`XcdrException: sequence length overflow:
   4294967289`, i.e. a prior `int` re-read as a length). Encode is fine because
   `Xcdr2Writer` is a `class`. Fix is in codegen+binding: pass the reader by
   `ref` (or make `Xcdr2Reader` a class). This is why `Position location`,
   `sequence<Sample> history` are not in the runnable type.

2. **`map<…>` member** — *no codec.* The struct gets **no `TypeSupport` at all**;
   the generator emits a comment: `// Telemetry: no XCDR2 TypeSupport — contains
   a map/fixed/any member (no wire codec yet)` and a build-time warning
   `TypeObject emission skipped — UnsupportedTypeSpec("map (inline IDL map)")`.

3. **`union` member / standalone union** — *no `TypeSupport`.* The generator
   emits only a data record (`Discriminator` + `object? Value`) plus comments;
   no `IDdsTopicType<>` is produced, so it cannot be a topic type.

4. **Fixed array `long v[N]`** — *does not compile.* The generator emits scalar
   `w.WriteInt32(sample.Window)` (passing an `int[]` to an `int` parameter) and,
   on decode, assigns a scalar `int` to an `int[]` field. No array codec path.

5. **`typedef`-to-primitive member** — *does not compile.* For `typedef double X;`
   the field is typed as a generated `record class X(double Value)`, but the
   codec emits `w.WriteFloat64(sample.field)` / assigns `double` to the record —
   no conversions exist between the typedef record and the primitive.

6. **Nested `sequence<sequence<T>>`** — *does not compile.* The generator reuses
   the same temp names (`__seq`, `__mat`, `__item`, `__cnt`, `__list`, `__i`,
   `__e`) in the inner and outer sequence scopes, producing C# `CS0136`
   "a local … cannot be declared in this scope" errors.

What remains — primitives, enum, bounded string, bounded/unbounded
`sequence<primitive>`, `sequence<string>`, `@optional`, `@key`, `@appendable` —
**does** round-trip cleanly end-to-end, which this example proves.
