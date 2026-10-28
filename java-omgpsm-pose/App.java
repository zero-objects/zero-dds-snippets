import org.omg.dds.core.*;
import org.omg.dds.domain.*;
import org.omg.dds.pub.*;
import org.omg.dds.sub.*;
import org.omg.dds.topic.*;

public class App {
    public static void main(String[] args) throws Exception {
        // ---- EXACT website snippet (docs/java-omgdds.html "Quickstart (OMG PSM form)") ----
        // DDS-Java-PSM 1.0 §7.3.1: the implementation class is named by a system property.
        System.setProperty(ServiceEnvironment.IMPLEMENTATION_CLASS_NAME_PROPERTY,
                "org.zerodds.ServiceEnvironmentImpl");
        ServiceEnvironment env = ServiceEnvironment.createInstance(App.class.getClassLoader());
        DomainParticipantFactory factory = DomainParticipantFactory.getInstance(env);
        DomainParticipant dp = factory.createParticipant(0);

        Topic<Pose> topic = dp.createTopic("Telemetry", Pose.class);
        Publisher pub = dp.createPublisher();
        DataWriter<Pose> writer = pub.createDataWriter(topic);

        writer.write(new Pose("r1", 1.0, 2.0, 3.0));
        // -----------------------------------------------------------------------------------

        DataReader<Pose> reader = dp.createSubscriber().createDataReader(topic);
        writer.write(new Pose("r1", 1.0, 2.0, 3.0));
        Thread.sleep(50);
        try (Sample.Iterator<Pose> samples = reader.take()) {
            while (samples.hasNext()) {
                System.out.println("got: " + samples.next().getData());
            }
        }
    }
}
