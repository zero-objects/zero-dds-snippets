import org.omg.dds.core.ServiceEnvironment;
import org.omg.dds.domain.DomainParticipantFactory;
import org.omg.dds.sub.Sample;
import sensor_msgs.msg.Temperature;

public class App {
    public static void main(String[] args) throws Exception {
        var env = ServiceEnvironment.createInstance("org.zerodds.ServiceEnvironmentImpl");
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
        for (Sample<Temperature> s : reader.take()) {
            System.out.println("got: " + s.getData().getCelsius() + " / " + s.getData().getSensor_id());
        }
    }
}
