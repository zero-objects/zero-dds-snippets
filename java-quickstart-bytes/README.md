# java-quickstart-bytes

The barebones Java quickstart: bootstrap the ZeroDDS service via
`ServiceEnvironment.createInstance("org.zerodds.ServiceEnvironmentImpl")`, then create
a participant, a `byte[]`-typed `Topic` named `Chatter`, a `DataWriter` and a
`DataReader`. Writes `"hello"` as raw bytes and takes it back. Proves the untyped
(byte-array) path end-to-end without any IDL codegen.

## Build & run

Build the binding once, in the ZeroDDS checkout:

    cd crates/java-omgdds/java && JAVA_HOME=<jdk21> mvn -q install

Then compile `App.java` against the produced jar and run:

    JAR=crates/java-omgdds/java/target/omgdds-0.0.0.jar
    javac -cp "$JAR" -d out App.java
    java  -cp "out:$JAR" App

Expect `got: hello`.

## Reference

This is the runnable companion to the code snippet on
**[ZeroDDS Java binding](https://zerodds.de/bindings/java/)**.

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
