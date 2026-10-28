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

    JAR=$(ls crates/java-omgdds/java/target/omgdds-*.jar | head -1)
    javac -cp "$JAR" -d out App.java sensor_msgs/msg/Temperature.java
    java  -cp "out:$JAR" App

Expect `got: 23 / A7`.

## Reference

- Explained on the website: [Java](https://zerodds.org/bindings/java/)
- This example in the repository: [zero-dds-snippets/java-typed-temperature](https://github.com/zero-objects/zero-dds-snippets/tree/main/java-typed-temperature)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
