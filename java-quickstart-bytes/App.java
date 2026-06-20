import org.omg.dds.core.ServiceEnvironment;
import org.omg.dds.domain.DomainParticipant;
import org.omg.dds.domain.DomainParticipantFactory;
import org.omg.dds.topic.Topic;
import org.omg.dds.pub.DataWriter;
import org.omg.dds.sub.DataReader;

public class App {
    public static void main(String[] args) throws Exception {
        var env = ServiceEnvironment.createInstance("org.zerodds.ServiceEnvironmentImpl");
        var factory = DomainParticipantFactory.getInstance(env);
        var participant = factory.createParticipant(0);

        var topic = participant.createTopic("Chatter", byte[].class);
        var writer = participant.createPublisher().createDataWriter(topic);
        var reader = participant.createSubscriber().createDataReader(topic);

        Thread.sleep(500);

        writer.write("hello".getBytes());
        Thread.sleep(200);

        for (var sample : reader.take()) {
            System.out.println("got: " + new String(sample.getData()));
        }
    }
}
