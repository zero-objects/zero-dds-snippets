// IDL-conformance runnable example — ZeroDDS Java (OMG DDS Java-PSM) binding.
//
// Publishes one fully-populated sample of each working conformance type over a
// REAL DCPS DomainParticipant + Publisher/Subscriber loopback (not just an
// in-memory encode/decode), then takes it back and asserts every field survived
// the XCDR2 wire. The TypeSupport classes used here are emitted verbatim by
// `zerodds-idlc ... --java` from the conformance fixtures (see README).
//
// Features exercised end-to-end:
//   * 01_primitives — all 20 IDL primitive types (boolean, octet, char, wchar,
//                     short/long/long long, all signed+unsigned widths,
//                     int8..int64, float, double).
//   * 02_strings    — bounded + unbounded string and wstring.
//   * 11_optional   — @optional members (present AND absent), @appendable
//                     extensibility (DHEADER framing).
//
// Blocked combo features (enum, nested struct, sequence-of-struct, union, map,
// array, typedef, @key) are documented in README.md "Known limitations" with the
// precise codegen/runtime symptom — they are deliberately NOT faked here.

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

import java.util.List;
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
}
