# Java conformance: fresh code into the DDS API

This example generates current Java types, codecs and explicit typed DDS support
from the shared [Telemetry IDL](../rust/idl/20_mixed_combo.idl), then exercises
DomainParticipant, Publisher, Subscriber, DataWriter and DataReader using
`ZdTypeSupports.Support_combo_Telemetry.INSTANCE`.

This is a **local, in-process DDS API loopback**, not a UDP, cross-process or
cross-vendor measurement. RTPS is disabled for this experiment. An independent
codec pass checks both byte orders, key stability/mutation and strict rejection
of trailing bytes. Local delivery does not prove remote discovery or lifecycle.

## Build and run

JDK 17 or newer (`java`, `javac`, `jar`) must be on PATH, and the Java
binding jar built (`mvn -q install -DskipTests` in `crates/java-omgdds/java`
of the ZeroDDS checkout). Then:

```sh
./build-and-run.sh
```

Every option has a default: `--repo` is the checkout this directory sits in
(or `$ZERODDS_SRC`), `--idlc` is `zerodds-idlc` built from it, `--binding` is
`$ZERODDS_JAR` or the jar in `crates/java-omgdds/java/target`, and `--domain`
is 177. Invalid domain arguments fail instead of using a fallback.

Each invocation generates and compiles into a fresh temporary directory and
prints its location; nothing generated is committed. Unset `ZERODDS_RTPS` if
it enables RTPS.

Expected terminal marker after a successful run:

```text
JAVA_EXAMPLE_PASS: fresh Telemetry codecs and local DDS API loopback
```

## Coverage and remaining migration

Telemetry combines two keys, enum, typedef-to-double, nested structs in a
sequence, discriminated union, map entries, optional double and fixed array.
The current Java mapping uses public fields, arrays for sequences and map
entries, integer enum values and nullable boxed optionals. There are no old
bean setters, Optional wrappers, Java Map adapters or legacy TypeSupports here.

The previous README named four fixtures in the removed
tools/idlc/tests/conformance/fixtures directory. **The separate primitive and
string/wstring coverage has not yet been migrated.** This is an outstanding
example-coverage task, not a product restriction or a claim that those features
now pass through this example. The unchanged shared IDL above is
the source for this example's current assertions.

## Documentation and boundaries

The [Java integration guide](../../../documentation/05-integration/java.md)
explains the matching runtime, local Maven coordinates and explicit generated
descriptor contract. This README and App.java remain the executable recipe.

The [IDL compiler handbook](../../../documentation/04-idl/idlc-handbook.md)
describes code generation and the codec/DDS runtime boundary. That relative
link assumes this examples checkout lives inside the ZeroDDS workspace as
zerodds-examples/, as it does during internal integration.

Companion metadata comes from the shared AST lowering. Unsupported metadata
is not invented. A generated metadata method does not publish standard RTPS
PID_TYPE_INFORMATION by itself. Cross-process discovery, remote matching and
release-gate status require separate measured evidence.
