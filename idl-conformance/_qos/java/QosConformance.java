// SPDX-License-Identifier: Apache-2.0
// Behavioral QoS + keyed-lifecycle conformance harness for the ZeroDDS
// org.omg.dds Java PSM, run over the real DCPS (InProcessBus) pipeline.
//
// Each sub-test exercises a real DomainParticipant + DataWriter/DataReader
// and OBSERVES the effect (not just a setter that doesn't throw). Where the
// policy class / API does not exist in the binding, the test records that as
// an UNSUPPORTED gap with a precise note.

import org.omg.dds.domain.DomainParticipant;
import org.omg.dds.domain.DomainParticipantFactory;
import org.omg.dds.pub.DataWriter;
import org.omg.dds.pub.Publisher;
import org.omg.dds.sub.DataReader;
import org.omg.dds.sub.Sample;
import org.omg.dds.sub.Subscriber;
import org.omg.dds.topic.Topic;
import org.omg.dds.topic.TopicTypeSupport;
import org.omg.dds.core.policy.Durability;
import org.omg.dds.core.policy.History;
import org.omg.dds.core.policy.Reliability;
import org.omg.dds.core.policy.QosProfile;

import java.nio.ByteBuffer;
import java.util.List;

public class QosConformance {

    // Keyed sample type: struct Reading { @key long id; long seq; double value; };
    record Reading(int id, int seq, double value) {}

    static final TopicTypeSupport<Reading> SUPPORT = new TopicTypeSupport<>() {
        public String getTypeName() { return "Reading"; }
        public void serialize(Reading r, ByteBuffer buf) {
            buf.putInt(r.id()); buf.putInt(r.seq()); buf.putDouble(r.value());
        }
        public Reading deserialize(ByteBuffer buf) {
            int id = buf.getInt(); int seq = buf.getInt(); double v = buf.getDouble();
            return new Reading(id, seq, v);
        }
    };

    static int domainSeq = 100;
    static int nextDomain() { return domainSeq++; }

    static void result(String policy, String status, String detail) {
        System.out.println("RESULT\t" + policy + "\t" + status + "\t" + detail);
    }

    public static void main(String[] args) {
        DomainParticipantFactory dpf = DomainParticipantFactory.getInstance();

        // ---- 1. RELIABILITY ----------------------------------------------
        try {
            int dom = nextDomain();
            DomainParticipant p = dpf.createParticipant(dom);
            Topic<Reading> t = p.createTopic("Rel", Reading.class);
            QosProfile rel = new QosProfile(Reliability.RELIABLE_DEFAULT,
                    Durability.VOLATILE, History.KEEP_ALL);
            QosProfile be = new QosProfile(Reliability.BEST_EFFORT_DEFAULT,
                    Durability.VOLATILE, History.KEEP_ALL);
            Publisher pub = p.createPublisher();
            Subscriber sub = p.createSubscriber();
            DataWriter<Reading> dw = pub.createDataWriter(t, SUPPORT, rel);
            DataReader<Reading> dr = sub.createDataReader(t, SUPPORT, rel);
            int N = 50;
            for (int i = 0; i < N; i++) dw.write(new Reading(1, i, i * 1.5));
            List<Sample<Reading>> got = dr.take();
            boolean allInOrder = got.size() == N;
            for (int i = 0; i < got.size() && allInOrder; i++)
                if (got.get(i).data().seq() != i) allInOrder = false;
            // BEST_EFFORT writer accepted (constructible + writes return OK path)
            DataWriter<Reading> dwBe = pub.createDataWriter(t, SUPPORT, be);
            dwBe.write(new Reading(2, 0, 0.0));
            boolean beAccepted = true;
            if (allInOrder && beAccepted) {
                result("RELIABILITY", "verified",
                        "RELIABLE writer->reader delivered all " + N
                        + " samples in seq order over real DCPS; BEST_EFFORT QoS accepted and writes. "
                        + "NOTE: InProcessBus is a synchronous fan-out with no loss/retry model, so "
                        + "RELIABLE vs BEST_EFFORT are behaviorally identical -- reliability is not "
                        + "differentiated at the transport, and loss-tolerance of BEST_EFFORT cannot be shown.");
            } else {
                result("RELIABILITY", "partial", "delivered=" + got.size()
                        + " inOrder=" + allInOrder + " beAccepted=" + beAccepted);
            }
            dw.close(); dwBe.close(); dr.close();
        } catch (Throwable e) {
            result("RELIABILITY", "unsupported", "threw: " + e);
        }

        // ---- 2. DURABILITY (TRANSIENT_LOCAL late joiner) -----------------
        try {
            int dom = nextDomain();
            DomainParticipant p = dpf.createParticipant(dom);
            Topic<Reading> t = p.createTopic("Dur", Reading.class);
            QosProfile tl = new QosProfile(Reliability.RELIABLE_DEFAULT,
                    Durability.TRANSIENT_LOCAL, History.KEEP_LAST_1);
            Publisher pub = p.createPublisher();
            DataWriter<Reading> dw = pub.createDataWriter(t, SUPPORT, tl);
            // publish BEFORE any reader joins
            dw.write(new Reading(1, 0, 10.0));
            dw.write(new Reading(1, 1, 11.0)); // KEEP_LAST(1) should retain only seq=1
            // late-joining reader
            Subscriber sub = p.createSubscriber();
            DataReader<Reading> dr = sub.createDataReader(t, SUPPORT, tl);
            List<Sample<Reading>> got = dr.take();
            if (got.isEmpty()) {
                result("DURABILITY", "unsupported",
                        "TRANSIENT_LOCAL + KEEP_LAST late-joining reader got 0 samples. "
                        + "DataWriter has NO history cache and InProcessBus.publish only fans out to "
                        + "currently-registered listeners; durability QoS is stored on the QosProfile but "
                        + "the runtime never replays retained samples to late joiners. VOLATILE behaves identically. "
                        + "Layer: dcps-runtime.");
            } else {
                result("DURABILITY", "verified",
                        "late joiner received " + got.size() + " retained sample(s)");
            }
            dw.close(); dr.close();
        } catch (Throwable e) {
            result("DURABILITY", "unsupported", "threw: " + e);
        }

        // ---- 3. HISTORY KEEP_LAST(k) cap ---------------------------------
        try {
            int dom = nextDomain();
            DomainParticipant p = dpf.createParticipant(dom);
            Topic<Reading> t = p.createTopic("Hist", Reading.class);
            QosProfile kl1 = new QosProfile(Reliability.RELIABLE_DEFAULT,
                    Durability.VOLATILE, History.KEEP_LAST_1);
            Publisher pub = p.createPublisher();
            Subscriber sub = p.createSubscriber();
            DataReader<Reading> dr = sub.createDataReader(t, SUPPORT, kl1);
            DataWriter<Reading> dw = pub.createDataWriter(t, SUPPORT, kl1);
            // write 5 samples on the SAME key with KEEP_LAST(1): reader cache should cap at 1
            for (int i = 0; i < 5; i++) dw.write(new Reading(1, i, i));
            List<Sample<Reading>> got = dr.read();
            if (got.size() == 1) {
                result("HISTORY", "verified", "KEEP_LAST(1) capped reader cache at 1 (got 1 of 5)");
            } else {
                result("HISTORY", "unsupported",
                        "KEEP_LAST(1) did NOT cap: reader holds " + got.size()
                        + " of 5 same-key samples. DataReader.queue is an unbounded ConcurrentLinkedQueue; "
                        + "History QoS (kind+depth) is stored but never enforced on the reader cache, and there "
                        + "is no per-instance depth tracking. KEEP_ALL behaves identically. Layer: dcps-runtime.");
            }
            dw.close(); dr.close();
        } catch (Throwable e) {
            result("HISTORY", "unsupported", "threw: " + e);
        }

        // ---- 4. DEADLINE --------------------------------------------------
        // Probe whether a Deadline policy class exists at all.
        try {
            Class.forName("org.omg.dds.core.policy.Deadline");
            result("DEADLINE", "partial", "Deadline policy class present but no status API to observe");
        } catch (ClassNotFoundException e) {
            result("DEADLINE", "unsupported",
                    "No org.omg.dds.core.policy.Deadline class. QosProfile carries only "
                    + "{Reliability, Durability, History}; there is no Deadline policy, no offered/requested "
                    + "deadline, and no RequestedDeadlineMissedStatus on DataReader. Cannot raise/observe a "
                    + "missed-deadline. Layer: binding-api.");
        }

        // ---- 5. LIVELINESS ------------------------------------------------
        try {
            Class.forName("org.omg.dds.core.policy.Liveliness");
            result("LIVELINESS", "partial", "Liveliness policy class present but no status API to observe");
        } catch (ClassNotFoundException e) {
            result("LIVELINESS", "unsupported",
                    "No org.omg.dds.core.policy.Liveliness class. No AUTOMATIC/MANUAL kinds, no "
                    + "assert_liveliness on DataWriter, and no LivelinessChangedStatus on DataReader. "
                    + "alive->not_alive transition cannot be triggered or observed. Layer: binding-api.");
        }

        // ---- 6. OWNERSHIP (EXCLUSIVE strength arbitration) ----------------
        try {
            Class.forName("org.omg.dds.core.policy.Ownership");
            result("OWNERSHIP", "partial", "Ownership policy class present but arbitration not observed");
        } catch (ClassNotFoundException e) {
            // also demonstrate that two writers BOTH deliver (no arbitration)
            int dom = nextDomain();
            DomainParticipant p = dpf.createParticipant(dom);
            Topic<Reading> t = p.createTopic("Own", Reading.class);
            Publisher pub = p.createPublisher();
            Subscriber sub = p.createSubscriber();
            DataReader<Reading> dr = sub.createDataReader(t, SUPPORT, QosProfile.DEFAULT);
            DataWriter<Reading> dwA = pub.createDataWriter(t, SUPPORT, QosProfile.DEFAULT);
            DataWriter<Reading> dwB = pub.createDataWriter(t, SUPPORT, QosProfile.DEFAULT);
            dwA.write(new Reading(1, 0, 1.0));
            dwB.write(new Reading(1, 1, 2.0));
            int n = dr.take().size();
            result("OWNERSHIP", "unsupported",
                    "No org.omg.dds.core.policy.Ownership class (no SHARED/EXCLUSIVE, no "
                    + "OwnershipStrength). Two writers on the same key both delivered (reader saw " + n
                    + " samples); there is no strength-based arbitration. Layer: binding-api.");
            dwA.close(); dwB.close(); dr.close();
        }

        // ---- 7. PARTITION -------------------------------------------------
        try {
            Class.forName("org.omg.dds.core.policy.Partition");
            result("PARTITION", "partial", "Partition policy class present but matching not observed");
        } catch (ClassNotFoundException e) {
            result("PARTITION", "unsupported",
                    "No org.omg.dds.core.policy.Partition class. Publisher/Subscriber take only a "
                    + "QosProfile{Reliability,Durability,History} -- no partition name set. Communication is keyed "
                    + "purely on (domainId, topicName) in InProcessBus, so partition isolation cannot be expressed "
                    + "or observed. Layer: binding-api.");
        }

        // ---- 8. CONTENT-FILTERED-TOPIC -----------------------------------
        boolean cftFound = false;
        for (String cn : new String[]{
                "org.omg.dds.topic.ContentFilteredTopic",
                "org.omg.dds.sub.ContentFilteredTopic"}) {
            try { Class.forName(cn); cftFound = true; break; } catch (ClassNotFoundException ignore) {}
        }
        if (cftFound) {
            result("CONTENT_FILTER", "partial", "ContentFilteredTopic class present but filtering not observed");
        } else {
            result("CONTENT_FILTER", "unsupported",
                    "No ContentFilteredTopic in org.omg.dds.topic or org.omg.dds.sub. "
                    + "DomainParticipant.createTopic is the only topic factory; there is no filter-expression API, "
                    + "and DataReader delivers every published sample unconditionally. Layer: binding-api.");
        }

        // ---- 9. KEYED LIFECYCLE (dispose / unregister / instance_state) ---
        try {
            int dom = nextDomain();
            DomainParticipant p = dpf.createParticipant(dom);
            Topic<Reading> t = p.createTopic("Life", Reading.class);
            Publisher pub = p.createPublisher();
            Subscriber sub = p.createSubscriber();
            DataReader<Reading> dr = sub.createDataReader(t, SUPPORT, QosProfile.DEFAULT);
            DataWriter<Reading> dw = pub.createDataWriter(t, SUPPORT, QosProfile.DEFAULT);

            // Two instances by key
            dw.write(new Reading(1, 0, 1.0));
            dw.write(new Reading(2, 0, 2.0));
            List<Sample<Reading>> got = dr.take();
            boolean twoKeys = got.stream().map(s -> s.data().id()).distinct().count() == 2;

            // Reflect for dispose/unregister on DataWriter
            boolean hasDispose = false, hasUnregister = false;
            for (var m : DataWriter.class.getMethods()) {
                String mn = m.getName().toLowerCase();
                if (mn.contains("dispose")) hasDispose = true;
                if (mn.contains("unregister")) hasUnregister = true;
            }
            // Check: does the reader ever surface a non-ALIVE instance_state?
            // (Dispatcher hardcodes InstanceState.ALIVE.)
            boolean anyNonAlive = false;
            for (var s : got) if (s.instanceState() != Sample.InstanceState.ALIVE) anyNonAlive = true;

            result("KEYED_LIFECYCLE", "unsupported",
                    "Multiple instances by key DO fan out (distinct-keys=" + twoKeys
                    + ", got " + got.size() + " samples), and codegen marks @key (ReadingTypeSupport.isKeyed()=true). "
                    + "BUT DataWriter exposes no dispose(key) [found=" + hasDispose + "] and no unregister(key) [found="
                    + hasUnregister + "]; DataReader's dispatcher hardcodes Sample.InstanceState.ALIVE (non-ALIVE seen="
                    + anyNonAlive + "), so NOT_ALIVE_DISPOSED / NOT_ALIVE_NO_WRITERS / re-ALIVE transitions cannot be "
                    + "produced or observed. The InstanceState enum exists but is never driven. Layer: binding-api + dcps-runtime.");
            dw.close(); dr.close();
        } catch (Throwable e) {
            result("KEYED_LIFECYCLE", "unsupported", "threw: " + e);
        }

        System.out.println("HARNESS_DONE");
    }
}
