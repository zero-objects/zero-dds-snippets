// Verified snippet from website/bindings/java/index.html — the java8_compat
// (Java-8: abstract class + private ctor pseudo-sealing) form of the same union.
public abstract class U8 {
    private U8() {}
    public static final class A extends U8 {
        private final int a;
        public A(int a) { this.a = a; }
        public int a() { return a; }
    }
    public static final class B extends U8 { /* ... */ }
}
