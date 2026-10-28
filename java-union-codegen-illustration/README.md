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

- Explained on the website: [Java](https://zerodds.org/bindings/java/)
- This example in the repository: [zero-dds-snippets/java-union-codegen-illustration](https://github.com/zero-objects/zero-dds-snippets/tree/main/java-union-codegen-illustration)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
