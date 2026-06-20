# java-typed-temperature

The typed-codegen quickstart: it writes and takes a `sensor_msgs.msg.Temperature`
bean — a stand-in for the class `idlc java` emits from
`struct { long celsius; string sensor_id; }`. Shows the default constructor plus
bean getters/setters with IDL field names left unchanged (`setCelsius`,
`setSensor_id`). Proves a roundtrip of a structured IDL type over topic `Temp`.

## Build & run

Build the binding once, in the ZeroDDS checkout:

    cd crates/java-omgdds/java && JAVA_HOME=<jdk21> mvn -q install

Then compile `App.java` together with the generated-type stub and run:

    JAR=crates/java-omgdds/java/target/omgdds-0.0.0.jar
    javac -cp "$JAR" -d out App.java sensor_msgs/msg/Temperature.java
    java  -cp "out:$JAR" App

Expect `got: 23 / A7`.

## Reference

This is the runnable companion to the code snippet on
**[ZeroDDS Java binding](https://zerodds.de/bindings/java/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
