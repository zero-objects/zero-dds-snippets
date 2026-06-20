import org.omg.dds.core.*;
import org.omg.dds.domain.*;
import org.omg.dds.pub.*;
import org.omg.dds.sub.*;
import org.omg.dds.topic.*;

public class App {
    public static void main(String[] args) throws Exception {
        // ---- EXACT website snippet (docs/java-omgdds.html "Quickstart (OMG PSM form)") ----
        DomainParticipantFactory factory = DomainParticipantFactory.getInstance();
        DomainParticipant dp = factory.createParticipant(0);

        Topic<Pose> topic = dp.createTopic("Telemetry", Pose.class);
        Publisher pub = dp.createPublisher();
        DataWriter<Pose> writer = pub.createDataWriter(topic);

        writer.write(new Pose("r1", 1.0, 2.0, 3.0));
        // -----------------------------------------------------------------------------------

        DataReader<Pose> reader = dp.createSubscriber().createDataReader(topic);
        writer.write(new Pose("r1", 1.0, 2.0, 3.0));
        Thread.sleep(50);
        for (Sample<Pose> s : reader.take()) {
            System.out.println("got: " + s.getData());
        }
    }
}
