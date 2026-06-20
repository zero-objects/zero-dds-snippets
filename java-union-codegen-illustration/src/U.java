// Verified snippet from website/bindings/java/index.html — "Java-8-Compat-Mode"
// section: the Java-17 (sealed interface + record) form of a generated union.
public sealed interface U permits U.A, U.B {
    record A(int a) implements U {}
    record B(String b) implements U {}
}
