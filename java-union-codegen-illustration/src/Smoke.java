// Minimal runnable smoke that exercises both generated-union forms.
public class Smoke {
    public static void main(String[] args) {
        U u = new U.A(7);
        String s = switch (u) {            // sealed-interface exhaustive switch
            case U.A a -> "A(" + a.a() + ")";
            case U.B b -> "B(" + b.b() + ")";
        };
        U8 v = new U8.A(42);               // java8_compat form
        int got = ((U8.A) v).a();
        if (!s.equals("A(7)") || got != 42) {
            System.err.println("FAIL"); System.exit(1);
        }
        System.out.println("OK: " + s + " / U8.A.a()=" + got);
    }
}
