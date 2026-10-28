// idl-conformance / _interop / java — the combo::Telemetry column.
//
//   java Interop ENCODE <out.bin>   write the canonical sample as raw XCDR2-LE
//   java Interop DECODE <in.bin>    decode, assert every field == canonical
//
// The types, codecs and TypeSupports are `zerodds-idlc generate --lang java`
// output of ../combo.idl (ZdTypes, ZdCodec, ZdTypeSupports) over
// runtime/java; values are CANONICAL.md.

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.Objects;
import org.zerodds.cdr.EndianMode;

public final class Interop {

    static ZdTypes.combo.Sample sample(int seq, double value) {
        var s = new ZdTypes.combo.Sample();
        s.seq = seq;
        s.value = value;
        return s;
    }

    static ZdTypes.ZdEntry_string_long entry(String key, int value) {
        var e = new ZdTypes.ZdEntry_string_long();
        e.key = key;
        e.value = value;
        return e;
    }

    static ZdTypes.combo.Telemetry canonical() {
        var x = new ZdTypes.combo.Telemetry();
        x.unitId = 4242;                                   // @key long
        x.region = "eu-central-1";                         // @key string<32>
        x.mode = ZdTypes.combo.Mode.MODE_ACTIVE;           // enum, index 1
        x.batteryCurrent = 13.75;                          // typedef double
        x.history = new ZdTypes.combo.Sample[] {sample(1, 0.5), sample(2, 1.5), sample(3, -2.25)};
        x.reading = new ZdTypes.combo.Reading();           // union switch(Mode) -> activeRate
        x.reading._d = ZdTypes.combo.Mode.MODE_ACTIVE;
        x.reading.activeRate = 60.0;
        x.counters = new ZdTypes.ZdEntry_string_long[] {entry("drops", 3), entry("packets", 100)};
        x.calibration = 0.001;                             // @optional double, present
        x.window = new int[] {10, 20, 30, 40};             // long[4]
        return x;
    }

    static int failures = 0;

    static void check(boolean ok, String what) {
        if (!ok) {
            failures++;
            System.err.println("  [FAIL] " + what);
        }
    }

    static void checkTelemetry(ZdTypes.combo.Telemetry g) {
        var w = canonical();
        check(g.unitId == w.unitId, "unitId");
        check(Objects.equals(g.region, w.region), "region");
        check(g.mode == w.mode, "mode");
        check(g.batteryCurrent == w.batteryCurrent, "batteryCurrent");
        boolean history = g.history.length == w.history.length;
        for (int i = 0; history && i < w.history.length; i++) {
            history = g.history[i].seq == w.history[i].seq && g.history[i].value == w.history[i].value;
        }
        check(history, "history");
        check(g.reading._d == w.reading._d && g.reading.activeRate == w.reading.activeRate, "reading");
        boolean counters = g.counters.length == w.counters.length;
        for (int i = 0; counters && i < w.counters.length; i++) {
            counters = Objects.equals(g.counters[i].key, w.counters[i].key) && g.counters[i].value == w.counters[i].value;
        }
        check(counters, "counters");
        check(Objects.equals(g.calibration, w.calibration), "calibration");
        check(Arrays.equals(g.window, w.window), "window");
    }

    public static void main(String[] args) throws Exception {
        if (args.length != 2 || !(args[0].equals("ENCODE") || args[0].equals("DECODE"))) {
            System.err.println("usage: Interop ENCODE <out.bin> | DECODE <in.bin>");
            System.exit(2);
        }
        Path path = Path.of(args[1]);
        var support = ZdTypeSupports.Support_combo_Telemetry.INSTANCE;
        if (args[0].equals("ENCODE")) {
            byte[] bytes = support.encode(canonical(), EndianMode.LITTLE_ENDIAN, 2);
            Files.write(path, bytes);
            System.out.println("ENCODE OK: " + bytes.length + " bytes -> " + path);
            return;
        }
        checkTelemetry(support.decode(Files.readAllBytes(path)));
        if (failures != 0) {
            System.out.println("DECODE FAILED: " + failures + " mismatch(es)");
            System.exit(1);
        }
        System.out.println("DECODE OK: every field equals canonical");
    }
}
