// SPDX-License-Identifier: Apache-2.0
// Fresh generated codec/descriptor integration through the local DDS API.
// This is not evidence of UDP or cross-process interoperability.
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import org.omg.dds.core.ServiceEnvironment;
import org.omg.dds.domain.DomainParticipantFactory;
import org.omg.dds.sub.Sample;
import org.zerodds.cdr.EndianMode;
import org.zerodds.cdr.XcdrException;

public final class App {
    private static void check(boolean condition, String why) {
        if (!condition) throw new AssertionError(why);
    }

    private static ZdTypes.combo.Telemetry sample(boolean optional) {
        var x = new ZdTypes.combo.Telemetry();
        x.setUnitId(7);
        x.setRegion("eu-central");
        x.setOperatingMode(ZdTypes.combo.Mode.MODE_ACTIVE);
        x.setBatteryCurrent(12.5);
        var s0 = new ZdTypes.combo.Sample();
        s0.setSeq(0); s0.setValue(1.5);
        var s1 = new ZdTypes.combo.Sample();
        s1.setSeq(1); s1.setValue(-2.25);
        x.setHistory(new ArrayList<>(List.of(s0, s1)));
        var latest = new ZdTypes.combo.Reading();
        latest.setDiscriminator(ZdTypes.combo.Mode.MODE_ACTIVE);
        latest.setActiveRate(98.6);
        x.setLatest(latest);
        var counters = new ZdTypes.ZdMap_string_long();
        counters.put("packets", 1000);
        counters.put("errors", 3);
        x.setCounters(counters);
        x.setCalibration(optional ? 0.0078125 : null);
        x.setWindow(new int[] {10, 20, 30, 40});
        return x;
    }

    private static void validate(ZdTypes.combo.Telemetry x, boolean optional) {
        check(x.getUnitId() == 7 && x.getRegion().equals("eu-central"), "key fields");
        check(x.getOperatingMode().value() == ZdTypes.combo.Mode._MODE_ACTIVE, "enum member");
        check(x.getBatteryCurrent() == 12.5, "typedef double");
        var history = new ArrayList<>(x.getHistory());
        check(history.size() == 2 && history.get(0).getSeq() == 0 && history.get(0).getValue() == 1.5
                && history.get(1).getSeq() == 1 && history.get(1).getValue() == -2.25, "nested struct sequence");
        check(x.getLatest().getDiscriminator().value() == ZdTypes.combo.Mode._MODE_ACTIVE
                && x.getLatest().getActiveRate() == 98.6, "union discriminator and arm");
        var entries = new ArrayList<Map.Entry<String, Integer>>(x.getCounters().entrySet());
        check(entries.size() == 2, "map length");
        check(entries.get(0).getKey().equals("packets") && entries.get(0).getValue() == 1000
                && entries.get(1).getKey().equals("errors") && entries.get(1).getValue() == 3, "map entries");
        check(optional ? Double.valueOf(0.0078125).equals(x.getCalibration()) : x.getCalibration() == null, "optional value");
        check(Arrays.equals(x.getWindow(), new int[] {10, 20, 30, 40}), "fixed array");
    }

    public static void main(String[] args) throws Exception {
        if (args.length != 1 || !args[0].matches("[0-9]{1,3}")) {
            throw new IllegalArgumentException("usage: App DOMAIN (0..232)");
        }
        int domain = Integer.parseInt(args[0]);
        if (domain > 232) throw new IllegalArgumentException("domain must be 0..232");
        check(!org.zerodds.rtps.DdsWireBridge.enabled(), "This example requires RTPS disabled");
        System.out.println("effective domain=" + domain + ", transport=in-process");
        var support = ZdTypeSupports.Support_combo_Telemetry.INSTANCE;
        check(support.writeRepresentation() == 2, "fresh XCDR2 descriptor required");
        check(support.getTypeName().equals("combo::Telemetry") && support.isKeyed(), "descriptor identity");
        for (EndianMode endian : EndianMode.values()) {
            var x = sample(true);
            byte[] bytes = support.encode(x, endian, 2);
            validate(support.decode(bytes, 0, bytes.length, endian, 2), true);
            try {
                support.decode(Arrays.copyOf(bytes, bytes.length + 1), 0, bytes.length + 1, endian, 2);
                throw new AssertionError("trailing byte accepted");
            } catch (XcdrException expected) {
                // Strict full-consumption contract, not a relaxed array comparison.
            }
        }
        var keyed = sample(true);
        byte[] key = support.keyHash(keyed);
        check(key.length == 16, "key hash size");
        keyed.setBatteryCurrent(99);
        check(Arrays.equals(key, support.keyHash(keyed)), "non-key changed instance identity");
        keyed.setUnitId(keyed.getUnitId() + 1);
        check(!Arrays.equals(key, support.keyHash(keyed)), "key mutation ignored");

        // DDS-Java-PSM 1.0 §7.3.1: the implementation class is named by a system property.
        System.setProperty(ServiceEnvironment.IMPLEMENTATION_CLASS_NAME_PROPERTY,
                "org.zerodds.ServiceEnvironmentImpl");
        var env = ServiceEnvironment.createInstance(App.class.getClassLoader());
        try (var participant = DomainParticipantFactory.getInstance(env).createParticipant(domain)) {
            var publisher = participant.createPublisher();
            var subscriber = participant.createSubscriber();
            // The class form: the binding finds the generated companion
            // ZdTypeSupports.Support_combo_Telemetry next to ZdTypes.
            try (var topic = participant.createTopic("FreshJavaTelemetry", ZdTypes.combo.Telemetry.class);
                 var reader = subscriber.createDataReader(topic);
                 var writer = publisher.createDataWriter(topic)) {
                for (boolean optional : new boolean[] {true, false}) {
                    var x = sample(optional);
                    var handle = writer.registerInstance(x);
                    check(writer.lookupInstance(x).equals(handle), "keyed local registration");
                    writer.write(x);
                    var got = new ArrayList<ZdTypes.combo.Telemetry>();
                    try (Sample.Iterator<ZdTypes.combo.Telemetry> samples = reader.take()) {
                        while (samples.hasNext()) {
                            got.add(samples.next().getData());
                        }
                    }
                    check(got.size() == 1 && got.get(0) != null, "exactly one valid typed sample");
                    validate(got.get(0), optional);
                }
            }
        }
        System.out.println("JAVA_EXAMPLE_PASS: fresh Telemetry codecs and local DDS API loopback");
    }
}
