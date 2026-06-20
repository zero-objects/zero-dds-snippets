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

    JAR=crates/java-omgdds/java/target/omgdds-0.0.0.jar
    javac -cp "$JAR" -d out App.java Pose.java
    java  -cp "out:$JAR" App

Expect `got: Pose[id=r1, x=1.0, y=2.0, z=3.0]`.

## Reference

This is the runnable companion to the code snippet on
**[ZeroDDS for Java (OMG DDS PSM)](https://zerodds.de/docs/java-omgdds/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
