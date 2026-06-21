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
| `01_primitives`  | `Primitives` | **all 20 IDL primitive types**: `boolean`, `octet`, `char`, `wchar`, `short`/`unsigned short`, `long`/`unsigned long`, `long long`/`unsigned long long`, `float`, `double`, `int8`/`uint8`, `int16`/`uint16`, `int32`/`uint32`, `int64`/`uint64` |
| `02_strings`     | `Strings`    | bounded + unbounded `string`, bounded + unbounded `wstring` (non-ASCII: `é ☃ ❤`)   |
| `11_optional`    | `Optionals`  | `@optional` members **present AND absent**; `@appendable` extensibility (DHEADER framing) |

Each type is published via its generated `<Name>TypeSupport.INSTANCE` (a real
XCDR2 codec, not reflection) and read back via `DataReader.take()`.

## Build & run

The script builds the binding jar once (if missing), compiles, and runs:

```sh
./build-and-run.sh
```

Or the explicit steps (copy-paste):

```sh
REPO=/Users/sandrakessler/projects/zerodds
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
$IDLC generate $F/01_primitives.idl --java -o generated </dev/null
$IDLC generate $F/02_strings.idl    --java -o generated </dev/null
$IDLC generate $F/11_optional.idl   --java -o generated </dev/null
rm -f generated/TypeObjects.java   # see limitation (7) below
```

(`</dev/null` is required — the CLI reads stdin and otherwise hangs.)

## Known limitations

The headline goal was the combined `20_mixed_combo.idl` type (enum + nested
struct + sequence-of-struct + bounded string + union + map + optional + array +
typedef + `@key`). It does **not** round-trip end-to-end on the current Java
stack. The blocking features are excluded here rather than faked. Each was
verified with a real `zerodds-idlc … --java` run and, where it compiled, a real
loopback attempt:

1. **`sequence<struct>` / `sequence<string>` — hard codegen failure.**
   `zerodds-idlc generate 07_sequences.idl --java` aborts with
   `java codegen failed: unsupported IDL construct 'typesupport-encode
   element-type'`. No sources are produced at all. (This is the tracked "Bug J".)

2. **Nested struct member — decode underflow (no round-trip).**
   For `05_nested_structs.idl` the codegen *compiles*, but the generated decode
   does `MiddleTypeSupport.INSTANCE.decode(r.readBytes(r.remaining()))` for each
   nested member: the first nested member greedily consumes **all** remaining
   bytes (the encoder concatenates nested `encode()` outputs with no length
   prefix), so a second nested member throws
   `XcdrException: underflow: need 4, have 0`. Any struct embedding ≥1 other
   aggregate is affected.

3. **Enum-in-struct — does not compile.**
   `03_enums.idl`'s `UsesEnumTypeSupport` calls `ColorTypeSupport.INSTANCE.encode(…)`
   / `SparseTypeSupport.INSTANCE…`, but **no `ColorTypeSupport`/`SparseTypeSupport`
   class is emitted** (only the bare `Color`/`Sparse` enums). javac fails with
   `package ColorTypeSupport does not exist`.

4. **`typedef` member — does not compile (and would hit limitation 2).**
   `06_typedefs.idl`'s `UsesTypedefsTypeSupport` references
   `CurrentInAmpsTypeTypeSupport`, `LongSeqTypeSupport`, `Matrix3TypeSupport`,
   etc., none of which are emitted; it also uses the greedy `readBytes(remaining())`
   pattern from limitation 2.

5. **`union` — no TypeSupport emitted.**
   `09_unions.idl` emits the sealed-interface union types (`IntUnion`,
   `EnumUnion`) but **no `*TypeSupport`** class, so there is no wire codec to
   publish them with.

6. **`map<…>` — no TypeSupport emitted.**
   `13_maps.idl` (and `20_mixed_combo.idl`) emit a warning
   `TypeObject emission skipped — UnsupportedTypeSpec("map (inline IDL map)")`
   and produce **no** TypeSupport for the map-containing struct.

7. **Array-containing struct — no TypeSupport emitted.**
   `08_arrays.idl` emits `Arrays.java` (the POJO) but no `ArraysTypeSupport`
   (only the element `PointTypeSupport`).

8. **`TypeObjects.java` — does not compile (excluded).**
   Every `--java` run also drops a `TypeObjects.java` whose `byte[]` literals
   contain values like `0x9d`, which javac rejects as
   `incompatible types: possible lossy conversion from int to byte`. It is
   metadata only (TypeObject/TypeIdentifier registration), unused by the
   roundtrip, and is deleted in the regenerate step above.

Net effect: the Java binding round-trips structs of **primitives, strings/wstrings
(bounded + unbounded), and `@optional` of primitives/strings under `@appendable`**.
Member types that resolve to a *named aggregate* (nested struct, enum, typedef,
union, map, array-struct, sequence-of-struct) are blocked by real codegen/runtime
bugs and are reported, not stubbed.

## Reference

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
Companion to the **[ZeroDDS Java binding](https://zerodds.de/bindings/java/)**.
