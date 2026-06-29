# idl-conformance — Java

A **standalone, runnable** IDL-conformance example for the ZeroDDS **Java**
language binding (OMG DDS Java-PSM). It publishes one fully-populated sample of
each supported typed-IDL type over a **real DCPS** `DomainParticipant` +
`Publisher`/`Subscriber` loopback (not an in-memory encode/decode) and takes it
back, asserting every field survived the XCDR2 wire.

The `*TypeSupport` classes under `generated/` are emitted **verbatim** by the
ZeroDDS IDL compiler from the conformance fixtures in
`tools/idlc/tests/conformance/fixtures/`. Nothing in `generated/` is hand-edited.

## IDL features exercised (end-to-end, real wire)

| Fixture          | Type         | Features proven over the DCPS loopback                                              |
|------------------|--------------|------------------------------------------------------------------------------------|
| `01_primitives`   | `Primitives` | **all 20 IDL primitive types**: `boolean`, `octet`, `char`, `wchar`, `short`/`unsigned short`, `long`/`unsigned long`, `long long`/`unsigned long long`, `float`, `double`, `int8`/`uint8`, `int16`/`uint16`, `int32`/`uint32`, `int64`/`uint64` |
| `02_strings`      | `Strings`    | bounded + unbounded `string`, bounded + unbounded `wstring` (non-ASCII: `é ☃ ❤`)   |
| `11_optional`     | `Optionals`  | `@optional` members **present AND absent**; `@appendable` extensibility (DHEADER framing) |
| `20_mixed_combo`  | `Telemetry`  | the **combined keyed type** — `enum` member, `typedef double`, **nested struct in `sequence<Sample>`**, `union switch(enum)` member, **`map<string,long>`**, `@optional double`, **fixed array `long window[4]`**, two `@key` members (`long` + `string<32>`), all inside one `@appendable` struct |

Each type is published via its generated `<Name>TypeSupport.INSTANCE` (a real
XCDR2 codec, not reflection) and read back via `DataReader.take()`. The
`20_mixed_combo` `Telemetry` row exercises **every previously-blocked aggregate
member codec in a single sample** (enum, typedef, nested struct, sequence-of-
struct, union, map, optional, fixed array), proving they all survive the wire
together.

## Build & run

The script builds the binding jar once (if missing), compiles, and runs:

```sh
./build-and-run.sh
```

Or the explicit steps (copy-paste):

```sh
REPO=/path/to/zerodds
export JAVA_HOME=$(/usr/libexec/java_home -v 21)

# 1) Build the ZeroDDS Java binding jar once.
( cd "$REPO/crates/java-omgdds/java" && mvn -q -DskipTests install )
JAR="$REPO/crates/java-omgdds/java/target/omgdds-0.0.0.jar"

# 2) Compile generated types + idl-java runtime support + the App.
cd "$REPO/zerodds-examples/idl-conformance/java"
mkdir -p out
$JAVA_HOME/bin/javac -cp "$JAR" -d out \
    generated/conf/*.java runtime-support/*.java App.java

# 3) Run the real DCPS pub -> sub roundtrip.
$JAVA_HOME/bin/java -cp "out:$JAR" App
```

Expected output ends with:

```
ALL ROUNDTRIPS PASSED — typed IDL data survived the real DCPS wire.
```

### Why `runtime-support/`

The generated sources reference the `idl-java` runtime marker interface
`org.omg.dds.topic.TopicType<T>` and the `@org.zerodds.types.*` annotations.
These are **not** packaged in the binding jar — per
`crates/idl-java/runtime/README.md` they are shipped as drop-in source files for
the consumer project. `runtime-support/` is an unmodified copy of
`crates/idl-java/runtime/*.java`, compiled alongside the generated code.

## How to regenerate

```sh
IDLC=$REPO/target/debug/zerodds-idlc
F=$REPO/tools/idlc/tests/conformance/fixtures
$IDLC generate $F/01_primitives.idl  --java -o generated </dev/null
$IDLC generate $F/02_strings.idl     --java -o generated </dev/null
$IDLC generate $F/11_optional.idl    --java -o generated </dev/null
$IDLC generate $F/20_mixed_combo.idl --java -o generated </dev/null
```

`20_mixed_combo` prints `warning: TypeObject emission skipped —
UnsupportedTypeSpec("map (inline IDL map)")`: only the optional TypeObject
*metadata* is skipped for the map-containing `Telemetry`; the wire
`TelemetryTypeSupport` (map encode/decode included) **is** emitted and
round-trips. See "Known limitations" below.

(`</dev/null` is required — the CLI reads stdin and otherwise hangs.)

## Resolved (previously blocked, now round-tripping)

The headline goal — the combined `20_mixed_combo.idl` `Telemetry` type — now
generates, compiles, and round-trips end-to-end. The earlier "Bug J-cluster"
codegen defects are fixed in the IDL compiler, and every aggregate member that
used to be excluded is now asserted live in `App.roundTripTelemetry`:

- **`sequence<struct>` / `sequence<string>`** — no longer a hard codegen abort;
  `sequence<Sample>` of nested structs round-trips with proper length-delimited
  framing (the old greedy `readBytes(remaining())` is gone — nested elements are
  decoded from per-element delimited frames).
- **Nested struct member** — `combo.Sample` inside the sequence decodes via its
  own DHeader frame; no more `underflow: need 4, have 0`.
- **`enum` member** — `Mode` encodes/decodes as its `int` ordinal in-line.
- **`typedef` member** — `CurrentInAmpsType` (typedef `double`) round-trips.
- **`union switch(enum)` member** — `Reading` emits a real `ReadingTypeSupport`
  and round-trips (discriminator + selected arm).
- **`map<string,long>` member** — `Telemetry`'s `counters` emits a working
  encode/decode (count-prefixed key/value pairs) and survives the wire.
- **fixed array member** — `long window[4]` round-trips element-for-element.
- **`@key` members** — `long unitId` + `string<32> region` (the `keyHash` path
  compiles and the keyed topic publishes).
- **`TypeObjects.java`** — the `byte[]` literals now use proper `(byte)` casts;
  the file compiles and is kept (no longer deleted in the regenerate step).

## Known limitations

1. **TypeObject *metadata* is skipped for map-containing types (cosmetic).**
   `20_mixed_combo.idl` still prints
   `warning: TypeObject emission skipped — UnsupportedTypeSpec("map (inline IDL map)")`.
   The IDL compiler does not yet emit a `TypeObjects.java` registration entry for
   the map-bearing `Telemetry` type. This is **TypeObject/TypeIdentifier
   discovery metadata only** — the wire `TelemetryTypeSupport` (including the
   `map<string,long>` codec) is fully emitted and round-trips here, so the
   loopback is unaffected. It would matter only for remote XTypes type-discovery
   of a map member against a peer that has no matching local type.

Net effect: the Java binding now round-trips structs of **primitives,
strings/wstrings (bounded + unbounded), `@optional`**, *and* the full combined
`Telemetry` aggregate — **enum, typedef, nested struct, sequence-of-struct,
union, map, fixed array, and `@key`** — all over the real `@appendable` DCPS
wire. The only remaining gap is the optional TypeObject metadata emission for
map members (above), which does not affect the in-runtime loopback.

## Reference

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
Companion to the **[ZeroDDS Java binding](https://zerodds.de/bindings/java/)**.
