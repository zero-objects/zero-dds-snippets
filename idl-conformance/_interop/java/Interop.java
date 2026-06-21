// XCDR2 wire-level interop harness for the combo::Telemetry canonical sample.
//
// Two modes:
//   ENCODE          -> write the canonical sample's raw XCDR2-LE bytes to
//                      _interop/goldens/java.bin
//   DECODE <file>   -> read raw bytes, reconstruct, assert EVERY field equals
//                      the canonical sample; exit 0 on full match, nonzero with
//                      a diff otherwise.
//
// The bytes produced by TelemetryTypeSupport.encode(sample, LITTLE_ENDIAN) ARE
// the exact bytes DDS carries on the wire — no live pub/sub needed.

import combo.CurrentInAmpsType;
import combo.Mode;
import combo.Reading;
import combo.Sample;
import combo.Telemetry;
import combo.TelemetryTypeSupport;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

public final class Interop {

    private static final String GOLDEN =
        "/Users/sandrakessler/projects/zerodds/zerodds-examples/idl-conformance/_interop/goldens/java.bin";

    /** Build the ONE canonical sample. Identical values across every language. */
    static Telemetry canonical() {
        Telemetry x = new Telemetry();
        x.setUnitId(4242);                                         // @key long
        x.setRegion("eu-central-1");                               // @key string<32>
        x.setMode(Mode.MODE_ACTIVE);                               // enum index 1
        x.setBatteryCurrent(new CurrentInAmpsType(13.75));         // typedef double

        // sequence<Sample> history
        List<Sample> history = new ArrayList<>();
        history.add(mkSample(1, 0.5));
        history.add(mkSample(2, 1.5));
        history.add(mkSample(3, -2.25));
        x.setHistory(history);

        // union Reading switch(Mode): MODE_ACTIVE -> activeRate (double) = 60.0
        x.setReading(new Reading.ActiveRate(60.0));

        // map<string,long> counters, encoded key-sorted ("drops" < "packets")
        Map<String, Integer> counters = new LinkedHashMap<>();
        counters.put("drops", 3);
        counters.put("packets", 100);
        x.setCounters(counters);

        // @optional double calibration = PRESENT, 0.001
        x.setCalibration(Optional.of(0.001));

        // long window[4]
        x.setWindow(new int[] {10, 20, 30, 40});
        return x;
    }

    static Sample mkSample(int seq, double value) {
        Sample s = new Sample();
        s.setSeq(seq);
        s.setValue(value);
        return s;
    }

    public static void main(String[] args) throws Exception {
        if (args.length == 0) {
            System.err.println("usage: Interop ENCODE | DECODE <file>");
            System.exit(2);
        }
        switch (args[0]) {
            case "ENCODE" -> encode();
            case "DECODE" -> decode(args.length > 1 ? args[1] : GOLDEN);
            default -> {
                System.err.println("unknown mode: " + args[0]);
                System.exit(2);
            }
        }
    }

    static void encode() throws Exception {
        Telemetry x = canonical();
        byte[] bytes = TelemetryTypeSupport.INSTANCE.encode(
            x, org.zerodds.cdr.EndianMode.LITTLE_ENDIAN);
        Path out = Path.of(GOLDEN);
        Files.createDirectories(out.getParent());
        Files.write(out, bytes);
        System.out.println("ENCODE: wrote " + bytes.length + " bytes -> " + GOLDEN);
    }

    private static int failures = 0;

    static void eq(String what, Object expected, Object actual) {
        boolean ok = (expected == null) ? actual == null : expected.equals(actual);
        if (!ok) {
            failures++;
            System.out.println("  DIFF " + what + ": expected=" + expected + " actual=" + actual);
        } else {
            System.out.println("  ok   " + what + " = " + actual);
        }
    }

    static void decode(String file) throws Exception {
        byte[] bytes = Files.readAllBytes(Path.of(file));
        Telemetry r = TelemetryTypeSupport.INSTANCE.decode(bytes);
        Telemetry c = canonical();

        eq("unitId", c.getUnitId(), r.getUnitId());
        eq("region", c.getRegion(), r.getRegion());
        eq("mode", c.getMode(), r.getMode());
        eq("batteryCurrent", c.getBatteryCurrent().value(), r.getBatteryCurrent().value());

        // history
        List<Sample> ch = c.getHistory();
        List<Sample> rh = r.getHistory();
        eq("history.size", ch.size(), rh == null ? null : rh.size());
        if (rh != null && rh.size() == ch.size()) {
            for (int i = 0; i < ch.size(); i++) {
                eq("history[" + i + "].seq", ch.get(i).getSeq(), rh.get(i).getSeq());
                eq("history[" + i + "].value", ch.get(i).getValue(), rh.get(i).getValue());
            }
        }

        // union reading
        boolean rIsActive = r.getReading() instanceof Reading.ActiveRate;
        eq("reading is ActiveRate", Boolean.TRUE, rIsActive);
        if (rIsActive) {
            eq("reading.activeRate",
               ((Reading.ActiveRate) c.getReading()).activeRate(),
               ((Reading.ActiveRate) r.getReading()).activeRate());
        }

        // counters map
        Map<String, Integer> cc = c.getCounters();
        Map<String, Integer> rc = r.getCounters();
        eq("counters.size", cc.size(), rc == null ? null : rc.size());
        if (rc != null) {
            eq("counters[drops]", cc.get("drops"), rc.get("drops"));
            eq("counters[packets]", cc.get("packets"), rc.get("packets"));
        }

        // optional calibration
        eq("calibration", c.getCalibration(), r.getCalibration());

        // window array
        int[] cw = c.getWindow();
        int[] rw = r.getWindow();
        eq("window.length", cw.length, rw == null ? null : rw.length);
        if (rw != null && rw.length == cw.length) {
            for (int i = 0; i < cw.length; i++) {
                eq("window[" + i + "]", cw[i], rw[i]);
            }
        }

        System.out.println();
        if (failures == 0) {
            System.out.println("DECODE OK — every field matches the canonical sample.");
            System.exit(0);
        } else {
            System.out.println("DECODE FAILED — " + failures + " field(s) differ.");
            System.exit(1);
        }
    }
}
