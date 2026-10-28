import org.omg.dds.core.ServiceEnvironment;
import org.omg.dds.domain.DomainParticipantFactory;
import org.omg.dds.sub.Sample;
import sensor_msgs.msg.Temperature;

public class App {
    public static void main(String[] args) throws Exception {
        // DDS-Java-PSM 1.0 §7.3.1: the implementation class is named by a system property.
        System.setProperty(ServiceEnvironment.IMPLEMENTATION_CLASS_NAME_PROPERTY,
                "org.zerodds.ServiceEnvironmentImpl");
        var env = ServiceEnvironment.createInstance(App.class.getClassLoader());
        var factory = DomainParticipantFactory.getInstance(env);
        var participant = factory.createParticipant(0);

        // ---- EXACT website snippet (bindings/java/index.html "Codegen + Nutzen") ----
        var topic = participant.createTopic("Temp", Temperature.class);
        var writer = participant.createPublisher().createDataWriter(topic);
        var t = new Temperature();          // Default-Konstruktor
        t.setCelsius(23);
        t.setSensor_id("A7");               // Bean-Setter, IDL-Feldname unverändert
        writer.write(t);
        // ---------------------------------------------------------------------------

        var reader = participant.createSubscriber().createDataReader(topic);
        // (reader created after write only delivers future samples; re-write to prove roundtrip)
        writer.write(t);
        Thread.sleep(50);
        try (Sample.Iterator<Temperature> samples = reader.take()) {
            while (samples.hasNext()) {
                Sample<Temperature> s = samples.next();
                System.out.println("got: " + s.getData().getCelsius() + " / " + s.getData().getSensor_id());
            }
        }
    }
}
