// IDL-conformance runnable example — ZeroDDS Java (OMG DDS Java-PSM) binding.
//
// Publishes one fully-populated sample of each conformance type over a REAL
// DCPS DomainParticipant + Publisher/Subscriber loopback (not just an in-memory
// encode/decode), then takes it back and asserts every field survived the XCDR2
// wire. The TypeSupport classes used here are emitted verbatim by
// `zerodds-idlc ... --java` from the conformance fixtures (see README).
//
// Features exercised end-to-end:
//   * 01_primitives — all 20 IDL primitive types (boolean, octet, char, wchar,
//                     short/long/long long, all signed+unsigned widths,
//                     int8..int64, float, double).
//   * 02_strings    — bounded + unbounded string and wstring.
//   * 11_optional   — @optional members (present AND absent), @appendable
//                     extensibility (DHEADER framing).
//   * 20_mixed_combo — the COMBINED keyed Telemetry type: enum, typedef,
//                     nested struct, sequence-of-struct, union, map<string,long>,
//                     @optional, fixed array, bounded @key string — all in one
//                     @appendable struct, over the real wire.

import org.omg.dds.domain.DomainParticipant;
import org.omg.dds.domain.DomainParticipantFactory;
import org.omg.dds.pub.DataWriter;
import org.omg.dds.pub.Publisher;
import org.omg.dds.sub.DataReader;
import org.omg.dds.sub.Sample;
import org.omg.dds.sub.Subscriber;
import org.omg.dds.topic.Topic;

import conf.Optionals;
import conf.OptionalsTypeSupport;
import conf.Primitives;
import conf.PrimitivesTypeSupport;
import conf.Strings;
import conf.StringsTypeSupport;

import combo.Mode;
import combo.Reading;
import combo.Telemetry;
import combo.TelemetryTypeSupport;
// NB: combo.Sample collides with org.omg.dds.sub.Sample (imported above);
// it is referenced fully-qualified as combo.Sample throughout this file.

import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

public final class App {

    private static int failures = 0;

    private static void check(String what, boolean ok) {
        System.out.println((ok ? "  ok   " : "  FAIL ") + what);
        if (!ok) {
            failures++;
        }
    }

    public static void main(String[] args) {
        DomainParticipantFactory dpf = DomainParticipantFactory.getInstance();
        DomainParticipant participant = dpf.createParticipant(0);
        Publisher publisher = participant.createPublisher();
        Subscriber subscriber = participant.createSubscriber();

        roundTripPrimitives(participant, publisher, subscriber);
        roundTripStrings(participant, publisher, subscriber);
        roundTripOptionals(participant, publisher, subscriber);
        roundTripTelemetry(participant, publisher, subscriber);

        System.out.println();
        if (failures == 0) {
            System.out.println("ALL ROUNDTRIPS PASSED — typed IDL data survived the real DCPS wire.");
        } else {
            System.out.println(failures + " CHECK(S) FAILED");
            System.exit(1);
        }
    }

    // ---- 01_primitives : every IDL primitive type ---------------------------

    private static void roundTripPrimitives(
            DomainParticipant p, Publisher pub, Subscriber sub) {
        System.out.println("[01_primitives] all IDL primitive types");
        Topic<Primitives> topic = p.createTopic("ConformancePrimitives", Primitives.class);
        DataReader<Primitives> dr = sub.createDataReader(topic, PrimitivesTypeSupport.INSTANCE);
        DataWriter<Primitives> dw = pub.createDataWriter(topic, PrimitivesTypeSupport.INSTANCE);

        Primitives x = new Primitives();
        x.setB(true);
        x.setO((byte) 0xAB);
        x.setC('Z');
        x.setWc('é');          // 'é' — exercises wchar.
        x.setI16((short) -1234);
        x.setU16(60000);            // unsigned short -> int.
        x.setI32(-100000);
        x.setU32(4000000000L);      // unsigned long -> long.
        x.setI64(-9000000000L);
        x.setU64(9000000000L);      // in-range unsigned long long.
        x.setF32(3.14159f);
        x.setF64(2.718281828459045);
        x.setS8((byte) -7);
        x.setUn8((short) 200);      // uint8 -> short.
        x.setS16((short) -32000);
        x.setUn16(60000);           // uint16 -> int.
        x.setS32(123456);
        x.setUn32(4000000001L);     // uint32 -> long.
        x.setS64(987654321L);
        x.setUn64(1234567890123L);  // uint64 -> long.

        dw.write(x);
        List<Sample<Primitives>> got = dr.take();
        check("delivered exactly one sample", got.size() == 1);
        if (got.isEmpty()) {
            return;
        }
        Primitives r = got.get(0).getData();
        check("boolean b", r.getB() == x.getB());
        check("octet o", r.getO() == x.getO());
        check("char c", r.getC() == x.getC());
        check("wchar wc", r.getWc() == x.getWc());
        check("short i16", r.getI16() == x.getI16());
        check("unsigned short u16", r.getU16() == x.getU16());
        check("long i32", r.getI32() == x.getI32());
        check("unsigned long u32", r.getU32() == x.getU32());
        check("long long i64", r.getI64() == x.getI64());
        check("unsigned long long u64", r.getU64() == x.getU64());
        check("float f32", r.getF32() == x.getF32());
        check("double f64", r.getF64() == x.getF64());
        check("int8 s8", r.getS8() == x.getS8());
        check("uint8 un8", r.getUn8() == x.getUn8());
        check("int16 s16", r.getS16() == x.getS16());
        check("uint16 un16", r.getUn16() == x.getUn16());
        check("int32 s32", r.getS32() == x.getS32());
        check("uint32 un32", r.getUn32() == x.getUn32());
        check("int64 s64", r.getS64() == x.getS64());
        check("uint64 un64", r.getUn64() == x.getUn64());
    }

    // ---- 02_strings : bounded/unbounded string + wstring --------------------

    private static void roundTripStrings(
            DomainParticipant p, Publisher pub, Subscriber sub) {
        System.out.println("[02_strings] bounded + unbounded string / wstring");
        Topic<Strings> topic = p.createTopic("ConformanceStrings", Strings.class);
        DataReader<Strings> dr = sub.createDataReader(topic, StringsTypeSupport.INSTANCE);
        DataWriter<Strings> dw = pub.createDataWriter(topic, StringsTypeSupport.INSTANCE);

        Strings x = new Strings();
        x.setUnbounded("an unbounded ASCII string of arbitrary length");
        x.setBounded("<= 32 chars");
        x.setWunbounded("unbounded wstring ☃ é");   // snowman + é.
        x.setWbounded("wbounded ❤");                     // <= 16, heart.

        dw.write(x);
        List<Sample<Strings>> got = dr.take();
        check("delivered exactly one sample", got.size() == 1);
        if (got.isEmpty()) {
            return;
        }
        Strings r = got.get(0).getData();
        check("unbounded string", x.getUnbounded().equals(r.getUnbounded()));
        check("bounded string<32>", x.getBounded().equals(r.getBounded()));
        check("unbounded wstring", x.getWunbounded().equals(r.getWunbounded()));
        check("bounded wstring<16>", x.getWbounded().equals(r.getWbounded()));
    }

    // ---- 11_optional : @optional present + absent, @appendable --------------

    private static void roundTripOptionals(
            DomainParticipant p, Publisher pub, Subscriber sub) {
        System.out.println("[11_optional] @optional present + absent");
        Topic<Optionals> topic = p.createTopic("ConformanceOptionals", Optionals.class);
        DataReader<Optionals> dr = sub.createDataReader(topic, OptionalsTypeSupport.INSTANCE);
        DataWriter<Optionals> dw = pub.createDataWriter(topic, OptionalsTypeSupport.INSTANCE);

        // Sample 1: both optionals present.
        Optionals present = new Optionals();
        present.setRequired(42);
        present.setMaybe(Optional.of(7));
        present.setNote(Optional.of("present"));
        dw.write(present);

        List<Sample<Optionals>> got1 = dr.take();
        check("sample-1 delivered", got1.size() == 1);
        if (!got1.isEmpty()) {
            Optionals r = got1.get(0).getData();
            check("required", r.getRequired() == 42);
            check("present @optional long maybe=7", r.getMaybe().equals(Optional.of(7)));
            check("present @optional string note", r.getNote().equals(Optional.of("present")));
        }

        // Sample 2: both optionals absent.
        Optionals absent = new Optionals();
        absent.setRequired(99);
        absent.setMaybe(Optional.empty());
        absent.setNote(Optional.empty());
        dw.write(absent);

        List<Sample<Optionals>> got2 = dr.take();
        check("sample-2 delivered", got2.size() == 1);
        if (!got2.isEmpty()) {
            Optionals r = got2.get(0).getData();
            check("required", r.getRequired() == 99);
            check("absent @optional long maybe", r.getMaybe().isEmpty());
            check("absent @optional string note", r.getNote().isEmpty());
        }
    }

    // ---- 20_mixed_combo : the full combined keyed Telemetry type ------------
    // enum + typedef + nested struct + sequence<struct> + union + map +
    // @optional + fixed array + bounded @key string, all in one @appendable
    // struct over the real DCPS wire.

    private static void roundTripTelemetry(
            DomainParticipant p, Publisher pub, Subscriber sub) {
        System.out.println("[20_mixed_combo] full Telemetry (enum+typedef+nested-seq+union+map+optional+array+key)");
        Topic<Telemetry> topic = p.createTopic("ConformanceTelemetry", Telemetry.class);
        DataReader<Telemetry> dr = sub.createDataReader(topic, TelemetryTypeSupport.INSTANCE);
        DataWriter<Telemetry> dw = pub.createDataWriter(topic, TelemetryTypeSupport.INSTANCE);

        Telemetry x = new Telemetry();
        x.setUnitId(7);                                          // @key long
        x.setRegion("eu-central");                               // @key string<32>
        x.setMode(Mode.MODE_ACTIVE);                             // enum member
        x.setBatteryCurrent(new combo.CurrentInAmpsType(12.5));  // typedef double

        // sequence<Sample> — nested struct in a sequence.
        combo.Sample s0 = new combo.Sample();
        s0.setSeq(0);
        s0.setValue(1.5);
        combo.Sample s1 = new combo.Sample();
        s1.setSeq(1);
        s1.setValue(-2.25);
        x.setHistory(List.of(s0, s1));

        // union Reading switch(Mode) — pick the MODE_ACTIVE arm (double).
        x.setReading(new Reading.ActiveRate(98.6));

        // map<string,long> — insertion-ordered.
        Map<String, Integer> counters = new LinkedHashMap<>();
        counters.put("packets", 1000);
        counters.put("errors", 3);
        x.setCounters(counters);

        // @optional double — present.
        x.setCalibration(Optional.of(0.0078125));

        // long window[4] — fixed array.
        x.setWindow(new int[] {10, 20, 30, 40});

        dw.write(x);
        List<Sample<Telemetry>> got = dr.take();
        check("delivered exactly one sample", got.size() == 1);
        if (got.isEmpty()) {
            return;
        }
        Telemetry r = got.get(0).getData();

        check("@key long unitId", r.getUnitId() == x.getUnitId());
        check("@key string<32> region", x.getRegion().equals(r.getRegion()));
        check("enum mode", r.getMode() == Mode.MODE_ACTIVE);
        check("typedef double batteryCurrent",
                r.getBatteryCurrent().value() == x.getBatteryCurrent().value());

        // sequence<Sample> nested struct.
        check("sequence<Sample> size", r.getHistory() != null && r.getHistory().size() == 2);
        if (r.getHistory() != null && r.getHistory().size() == 2) {
            combo.Sample r0 = r.getHistory().get(0);
            combo.Sample r1 = r.getHistory().get(1);
            check("history[0].seq", r0.getSeq() == 0);
            check("history[0].value", r0.getValue() == 1.5);
            check("history[1].seq", r1.getSeq() == 1);
            check("history[1].value", r1.getValue() == -2.25);
        }

        // union Reading.
        check("union reading is ActiveRate", r.getReading() instanceof Reading.ActiveRate);
        if (r.getReading() instanceof Reading.ActiveRate ar) {
            check("union activeRate value", ar.activeRate() == 98.6);
        }

        // map<string,long>.
        check("map size", r.getCounters() != null && r.getCounters().size() == 2);
        if (r.getCounters() != null) {
            check("map[packets]=1000", Integer.valueOf(1000).equals(r.getCounters().get("packets")));
            check("map[errors]=3", Integer.valueOf(3).equals(r.getCounters().get("errors")));
        }

        // @optional present.
        check("@optional calibration present", r.getCalibration().isPresent());
        check("@optional calibration value",
                r.getCalibration().equals(Optional.of(0.0078125)));

        // fixed array long window[4].
        check("array window[4]", Arrays.equals(r.getWindow(), new int[] {10, 20, 30, 40}));
    }
}
