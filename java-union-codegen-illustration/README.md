# java-union-codegen-illustration

Verifies the union-codegen illustration snippet from
`website/bindings/java/index.html` (Java-8-Compat-Mode section): the
`sealed interface`+`record` (Java 17) form and the `abstract class`+private-ctor
(java8_compat) form of a generated IDL union. Pure Java language constructs — no
ZeroDDS binding API involved.

Build & run:
    javac -d out src/*.java
    java -cp out Smoke
