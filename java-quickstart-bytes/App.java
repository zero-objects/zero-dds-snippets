import org.omg.dds.core.ServiceEnvironment;
import org.omg.dds.domain.DomainParticipantFactory;
import org.omg.dds.sub.Sample;

public class App {
    public static void main(String[] args) throws Exception {
        // DDS-Java-PSM 1.0 §7.3.1: the implementation class is named by a system property.
        System.setProperty(ServiceEnvironment.IMPLEMENTATION_CLASS_NAME_PROPERTY,
                "org.zerodds.ServiceEnvironmentImpl");
        var env = ServiceEnvironment.createInstance(App.class.getClassLoader());
        var factory = DomainParticipantFactory.getInstance(env);
        var participant = factory.createParticipant(0);

        var topic = participant.createTopic("Chatter", byte[].class);
        var writer = participant.createPublisher().createDataWriter(topic);
        var reader = participant.createSubscriber().createDataReader(topic);

        Thread.sleep(500);

        writer.write("hello".getBytes());
        Thread.sleep(200);

        try (Sample.Iterator<byte[]> samples = reader.take()) {
            while (samples.hasNext()) {
                System.out.println("got: " + new String(samples.next().getData()));
            }
        }
    }
}
