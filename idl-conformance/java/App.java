// SPDX-License-Identifier: Apache-2.0
// Fresh generated codec/descriptor integration through the local DDS API.
// This is not evidence of UDP or cross-process interoperability.
import java.util.Arrays;
import org.omg.dds.core.ReturnCode;
import org.omg.dds.domain.DomainParticipantFactory;
import org.zerodds.cdr.EndianMode;
import org.zerodds.cdr.XcdrException;

public final class App {
    private static void check(boolean condition, String why) {
        if (!condition) throw new AssertionError(why);
    }

    private static ZdTypes.combo.Telemetry sample(boolean optional) {
        var x = new ZdTypes.combo.Telemetry();
        x.unitId = 7;
        x.region = "eu-central";
        x.operatingMode = ZdTypes.combo.Mode.MODE_ACTIVE;
        x.batteryCurrent = 12.5;
        var s0 = new ZdTypes.combo.Sample();
        s0.seq = 0; s0.value = 1.5;
        var s1 = new ZdTypes.combo.Sample();
        s1.seq = 1; s1.value = -2.25;
        x.history = new ZdTypes.combo.Sample[] {s0, s1};
        x.latest = new ZdTypes.combo.Reading();
        x.latest._d = ZdTypes.combo.Mode.MODE_ACTIVE;
        x.latest.activeRate = 98.6;
        var packets = new ZdTypes.ZdEntry_string_long();
        packets.key = "packets"; packets.value = 1000;
        var errors = new ZdTypes.ZdEntry_string_long();
        errors.key = "errors"; errors.value = 3;
        x.counters = new ZdTypes.ZdEntry_string_long[] {packets, errors};
        x.calibration = optional ? 0.0078125 : null;
        x.window = new int[] {10, 20, 30, 40};
        return x;
    }

    private static void validate(ZdTypes.combo.Telemetry x, boolean optional) {
        check(x.unitId == 7 && x.region.equals("eu-central"), "key fields");
        check(x.operatingMode == ZdTypes.combo.Mode.MODE_ACTIVE, "enum member");
        check(x.batteryCurrent == 12.5, "typedef double");
        check(x.history.length == 2 && x.history[0].seq == 0 && x.history[0].value == 1.5
                && x.history[1].seq == 1 && x.history[1].value == -2.25, "nested struct sequence");
        check(x.latest._d == ZdTypes.combo.Mode.MODE_ACTIVE && x.latest.activeRate == 98.6, "union discriminator and arm");
        check(x.counters.length == 2, "map length");
        check(x.counters[0].key.equals("packets") && x.counters[0].value == 1000
                && x.counters[1].key.equals("errors") && x.counters[1].value == 3, "map entries");
        check(optional ? Double.valueOf(0.0078125).equals(x.calibration) : x.calibration == null, "optional value");
        check(Arrays.equals(x.window, new int[] {10, 20, 30, 40}), "fixed array");
    }

    public static void main(String[] args) {
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
        keyed.batteryCurrent = 99;
        check(Arrays.equals(key, support.keyHash(keyed)), "non-key changed instance identity");
        keyed.unitId++;
        check(!Arrays.equals(key, support.keyHash(keyed)), "key mutation ignored");

        try (var participant = DomainParticipantFactory.getInstance().createParticipant(domain)) {
            var publisher = participant.createPublisher();
            var subscriber = participant.createSubscriber();
            try (var topic = participant.createTopic("FreshJavaTelemetry", ZdTypes.combo.Telemetry.class);
                 var reader = subscriber.createDataReader(topic, support);
                 var writer = publisher.createDataWriter(topic, support)) {
                for (boolean optional : new boolean[] {true, false}) {
                    var x = sample(optional);
                    var handle = writer.registerInstance(x);
                    check(writer.lookupInstance(x).equals(handle), "keyed local registration");
                    check(writer.write(x) == ReturnCode.OK, "write return code");
                    var got = reader.take();
                    check(got.size() == 1 && got.get(0).data() != null, "exactly one valid typed sample");
                    validate(got.get(0).data(), optional);
                }
            }
        }
        System.out.println("JAVA_EXAMPLE_PASS: fresh Telemetry codecs and local DDS API loopback");
    }
}
