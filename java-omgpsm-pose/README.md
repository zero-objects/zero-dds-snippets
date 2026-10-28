# java-omgpsm-pose

Runs the OMG-PSM-form quickstart: it drives ZeroDDS through the standard `org.omg.dds`
API (`DomainParticipantFactory` → `Topic<Pose>` → `Publisher`/`DataWriter`,
`Subscriber`/`DataReader`). `Pose` is a Java `record` standing in for the
`idl-java`-generated type — `ReflectionTypeSupport` marshals it via its canonical
`(String, double, double, double)` constructor. Proves a typed write/take roundtrip
on topic `Telemetry` with no hand-written serialization.

## Build & run

Build the binding once, in the ZeroDDS checkout:

    cd crates/java-omgdds/java && JAVA_HOME=<jdk21> mvn -q install

Then compile `App.java` + `Pose.java` against the produced jar and run:

    JAR=$(ls crates/java-omgdds/java/target/omgdds-*.jar | head -1)
    javac -cp "$JAR" -d out App.java Pose.java
    java  -cp "out:$JAR" App

Expect `got: Pose[id=r1, x=1.0, y=2.0, z=3.0]`.

## Reference

- Explained on the website: [zerodds-java-omgdds](https://zerodds.org/docs/java-omgdds.html)
- This example in the repository: [zero-dds-snippets/java-omgpsm-pose](https://github.com/zero-objects/zero-dds-snippets/tree/main/java-omgpsm-pose)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
