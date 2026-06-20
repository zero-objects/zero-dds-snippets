# java-union-codegen-illustration

Illustrates the two Java forms `idlc java` emits for a generated IDL union: the
Java-17 `sealed interface` + `record` form (`U` permitting `U.A`/`U.B`) and the
`java8_compat` `abstract class` + private-ctor pseudo-sealed form (`U8`). `Smoke.java`
exercises both — an exhaustive `switch` over the sealed interface and a cast on the
Java-8 form. Pure Java language constructs only; no ZeroDDS binding API is involved.

## Build & run

No binding jar is needed — these are plain Java sources:

    javac -d out src/*.java
    java -cp out Smoke

Expect `OK: A(7) / U8.A.a()=42`.

## Reference

This is the runnable companion to the code snippet on
**[ZeroDDS Java binding](https://zerodds.de/bindings/java/)** (Java-8-Compat-Mode section).

Part of **[ZeroDDS](https://zerodds.de)** — the pure-Rust OMG DDS implementation.
