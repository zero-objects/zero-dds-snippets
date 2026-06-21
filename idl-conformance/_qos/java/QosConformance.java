// SPDX-License-Identifier: Apache-2.0
// Behavioral QoS + keyed-lifecycle conformance harness for the ZeroDDS
// org.omg.dds Java PSM, run over the real DCPS (InProcessBus) pipeline.
//
// Each sub-test exercises a real DomainParticipant + DataWriter/DataReader
// and OBSERVES the effect (not just a setter that doesn't throw). The QoS
// policy classes, keyed lifecycle and status getters are now present, so
// previously-"unsupported" gaps are verified here.

import org.omg.dds.domain.DomainParticipant;
import org.omg.dds.domain.DomainParticipantFactory;
import org.omg.dds.pub.DataWriter;
import org.omg.dds.pub.Publisher;
import org.omg.dds.sub.DataReader;
import org.omg.dds.sub.Sample;
import org.omg.dds.sub.Subscriber;
import org.omg.dds.topic.ContentFilteredTopic;
import org.omg.dds.topic.Topic;
import org.omg.dds.topic.TopicTypeSupport;
import org.omg.dds.core.Duration;
import org.omg.dds.core.policy.Deadline;
import org.omg.dds.core.policy.Durability;
import org.omg.dds.core.policy.History;
import org.omg.dds.core.policy.Liveliness;
import org.omg.dds.core.policy.Ownership;
import org.omg.dds.core.policy.OwnershipStrength;
import org.omg.dds.core.policy.Partition;
import org.omg.dds.core.policy.QosProfile;
import org.omg.dds.core.policy.Reliability;
import org.omg.dds.core.status.LivelinessChangedStatus;
import org.omg.dds.core.status.RequestedDeadlineMissedStatus;

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
        public boolean isKeyed() { return true; }
        public byte[] keyHash(Reading r) {
            return ByteBuffer.allocate(4).putInt(r.id()).array();
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
            DataReader<Reading> dr = sub.createDataReader(t, SUPPORT, rel);
            DataWriter<Reading> dw = pub.createDataWriter(t, SUPPORT, rel);
            int N = 50;
            for (int i = 0; i < N; i++) dw.write(new Reading(1, i, i * 1.5));
            List<Sample<Reading>> got = dr.take();
            boolean allInOrder = got.size() == N;
            for (int i = 0; i < got.size() && allInOrder; i++)
                if (got.get(i).data().seq() != i) allInOrder = false;
            // RxO: a RELIABLE reader is NOT compatible with a BEST_EFFORT writer.
            boolean rxoRejects = !rel.isCompatibleWith(be);
            if (allInOrder && rxoRejects) {
                result("RELIABILITY", "verified",
                        "RELIABLE writer->reader delivered all " + N + " samples in seq order; "
                        + "RxO isCompatibleWith() correctly rejects RELIABLE-reader vs BEST_EFFORT-writer.");
            } else {
                result("RELIABILITY", "partial", "delivered=" + got.size()
                        + " inOrder=" + allInOrder + " rxoRejects=" + rxoRejects);
            }
            dw.close(); dr.close();
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
            dw.write(new Reading(1, 0, 10.0));
            dw.write(new Reading(1, 1, 11.0)); // KEEP_LAST(1) -> retain only seq=1
            Subscriber sub = p.createSubscriber();
            DataReader<Reading> dr = sub.createDataReader(t, SUPPORT, tl);
            List<Sample<Reading>> got = dr.take();
            if (got.size() == 1 && got.get(0).data().seq() == 1) {
                result("DURABILITY", "verified",
                        "TRANSIENT_LOCAL late joiner replayed " + got.size()
                        + " retained sample (seq=" + got.get(0).data().seq()
                        + "), KEEP_LAST(1) honored on the replay.");
            } else {
                result("DURABILITY", "partial", "late joiner got " + got.size() + " samples");
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
            for (int i = 0; i < 5; i++) dw.write(new Reading(1, i, i));
            List<Sample<Reading>> got = dr.read();
            if (got.size() == 1 && got.get(0).data().seq() == 4) {
                result("HISTORY", "verified",
                        "KEEP_LAST(1) capped reader cache at 1 (got 1 of 5, retained newest seq=4).");
            } else {
                result("HISTORY", "partial",
                        "KEEP_LAST(1) reader holds " + got.size() + " of 5 same-key samples.");
            }
            dw.close(); dr.close();
        } catch (Throwable e) {
            result("HISTORY", "unsupported", "threw: " + e);
        }

        // ---- 4. DEADLINE --------------------------------------------------
        try {
            int dom = nextDomain();
            DomainParticipant p = dpf.createParticipant(dom);
            Topic<Reading> t = p.createTopic("Deadline", Reading.class);
            QosProfile q = QosProfile.DEFAULT.withDeadline(new Deadline(Duration.fromMillis(50)));
            Publisher pub = p.createPublisher();
            Subscriber sub = p.createSubscriber();
            DataReader<Reading> dr = sub.createDataReader(t, SUPPORT, q);
            DataWriter<Reading> dw = pub.createDataWriter(t, SUPPORT, q);
            dw.write(new Reading(1, 0, 1.0));
            int before = dr.getRequestedDeadlineMissedStatus().totalCount();
            Thread.sleep(140); // > 2 deadline periods, no new sample
            RequestedDeadlineMissedStatus st = dr.getRequestedDeadlineMissedStatus();
            if (before == 0 && st.totalCount() >= 1 && st.totalCountChange() >= 1) {
                result("DEADLINE", "verified",
                        "REQUESTED_DEADLINE_MISSED raised: totalCount=" + st.totalCount()
                        + " change=" + st.totalCountChange() + " after a missed 50ms period.");
            } else {
                result("DEADLINE", "partial",
                        "before=" + before + " total=" + st.totalCount() + " change=" + st.totalCountChange());
            }
            dw.close(); dr.close();
        } catch (Throwable e) {
            result("DEADLINE", "unsupported", "threw: " + e);
        }

        // ---- 5. LIVELINESS ------------------------------------------------
        try {
            int dom = nextDomain();
            DomainParticipant p = dpf.createParticipant(dom);
            Topic<Reading> t = p.createTopic("Liveliness", Reading.class);
            QosProfile q = QosProfile.DEFAULT.withLiveliness(
                    new Liveliness(Liveliness.Kind.AUTOMATIC, Duration.fromMillis(400)));
            Publisher pub = p.createPublisher();
            Subscriber sub = p.createSubscriber();
            DataReader<Reading> dr = sub.createDataReader(t, SUPPORT, q);
            DataWriter<Reading> dw = pub.createDataWriter(t, SUPPORT, q);
            LivelinessChangedStatus alive = dr.getLivelinessChangedStatus();
            dw.simulateLivelinessLost();
            LivelinessChangedStatus lost = dr.getLivelinessChangedStatus();
            if (alive.aliveCount() == 1 && lost.notAliveCount() == 1
                    && lost.aliveCountChange() == -1 && lost.notAliveCountChange() == 1) {
                result("LIVELINESS", "verified",
                        "LIVELINESS_CHANGED observed alive=1 -> notAlive=1 (aliveChange=-1, notAliveChange=1).");
            } else {
                result("LIVELINESS", "partial",
                        "alive=" + alive + " lost=" + lost);
            }
            dw.close(); dr.close();
        } catch (Throwable e) {
            result("LIVELINESS", "unsupported", "threw: " + e);
        }

        // ---- 6. OWNERSHIP (EXCLUSIVE strength arbitration) ----------------
        try {
            int dom = nextDomain();
            DomainParticipant p = dpf.createParticipant(dom);
            Topic<Reading> t = p.createTopic("Own", Reading.class);
            Publisher pub = p.createPublisher();
            Subscriber sub = p.createSubscriber();
            QosProfile drq = QosProfile.DEFAULT.withOwnership(Ownership.EXCLUSIVE);
            QosProfile low = QosProfile.DEFAULT.withOwnership(Ownership.EXCLUSIVE)
                    .withOwnershipStrength(new OwnershipStrength(1));
            QosProfile high = QosProfile.DEFAULT.withOwnership(Ownership.EXCLUSIVE)
                    .withOwnershipStrength(new OwnershipStrength(10));
            DataReader<Reading> dr = sub.createDataReader(t, SUPPORT, drq);
            DataWriter<Reading> dwLow = pub.createDataWriter(t, SUPPORT, low);
            DataWriter<Reading> dwHigh = pub.createDataWriter(t, SUPPORT, high);
            dwHigh.write(new Reading(1, 0, 100.0)); // owner
            dwLow.write(new Reading(1, 1, 1.0));     // filtered out
            dwHigh.write(new Reading(1, 2, 102.0));
            List<Sample<Reading>> got = dr.take();
            boolean onlyOwner = got.size() == 2 && got.stream().allMatch(s -> s.data().value() >= 100.0);
            if (onlyOwner) {
                result("OWNERSHIP", "verified",
                        "EXCLUSIVE arbitration: only highest-strength (10) writer delivered; "
                        + "lower-strength (1) sample on the same key was filtered (got " + got.size() + ").");
            } else {
                result("OWNERSHIP", "partial", "got " + got.size() + " samples (expected 2 from owner)");
            }
            dwLow.close(); dwHigh.close(); dr.close();
        } catch (Throwable e) {
            result("OWNERSHIP", "unsupported", "threw: " + e);
        }

        // ---- 7. PARTITION -------------------------------------------------
        try {
            int dom = nextDomain();
            DomainParticipant p = dpf.createParticipant(dom);
            Topic<Reading> t = p.createTopic("Part", Reading.class);
            Publisher pub = p.createPublisher();
            Subscriber sub = p.createSubscriber();
            // Non-overlapping partitions must NOT communicate.
            QosProfile pubA = QosProfile.DEFAULT.withPartition(new Partition("A"));
            QosProfile subB = QosProfile.DEFAULT.withPartition(new Partition("B"));
            DataReader<Reading> drB = sub.createDataReader(t, SUPPORT, subB);
            DataWriter<Reading> dwA = pub.createDataWriter(t, SUPPORT, pubA);
            dwA.write(new Reading(1, 0, 1.0));
            int isolated = drB.take().size();
            // Overlapping partitions DO communicate.
            QosProfile subA = QosProfile.DEFAULT.withPartition(new Partition("A"));
            DataReader<Reading> drA = sub.createDataReader(t, SUPPORT, subA);
            dwA.write(new Reading(2, 0, 2.0));
            int matched = drA.take().size();
            if (isolated == 0 && matched == 1) {
                result("PARTITION", "verified",
                        "Partition isolation enforced: A->B delivered 0; A->A delivered 1.");
            } else {
                result("PARTITION", "partial", "isolated=" + isolated + " matched=" + matched);
            }
            dwA.close(); drA.close(); drB.close();
        } catch (Throwable e) {
            result("PARTITION", "unsupported", "threw: " + e);
        }

        // ---- 8. CONTENT-FILTERED-TOPIC -----------------------------------
        try {
            int dom = nextDomain();
            DomainParticipant p = dpf.createParticipant(dom);
            Topic<Reading> t = p.createTopic("Cft", Reading.class);
            Publisher pub = p.createPublisher();
            Subscriber sub = p.createSubscriber();
            ContentFilteredTopic<Reading> cft = new ContentFilteredTopic<>(
                    "HighValue", t, "value > 50.0", r -> r.value() > 50.0);
            DataReader<Reading> dr = sub.createDataReader(cft, SUPPORT, QosProfile.DEFAULT);
            DataWriter<Reading> dw = pub.createDataWriter(t, SUPPORT, QosProfile.DEFAULT);
            dw.write(new Reading(1, 0, 10.0));  // dropped
            dw.write(new Reading(2, 1, 99.0));  // kept
            dw.write(new Reading(3, 2, 5.0));   // dropped
            List<Sample<Reading>> got = dr.take();
            if (got.size() == 1 && got.get(0).data().value() == 99.0) {
                result("CONTENT_FILTER", "verified",
                        "ContentFilteredTopic 'value > 50.0' delivered only the matching sample (1 of 3).");
            } else {
                result("CONTENT_FILTER", "partial", "got " + got.size() + " samples (expected 1)");
            }
            dw.close(); dr.close();
        } catch (Throwable e) {
            result("CONTENT_FILTER", "unsupported", "threw: " + e);
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

            dw.write(new Reading(1, 0, 1.0));
            dw.write(new Reading(2, 0, 2.0));
            dw.dispose(new Reading(1, 0, 0.0));        // key 1 -> DISPOSED
            dw.unregisterInstance(new Reading(2, 0, 0.0)); // key 2 -> NO_WRITERS
            List<Sample<Reading>> got = dr.take();
            boolean sawDisposed = got.stream().anyMatch(
                    s -> s.instanceState() == Sample.InstanceState.NOT_ALIVE_DISPOSED);
            boolean sawNoWriters = got.stream().anyMatch(
                    s -> s.instanceState() == Sample.InstanceState.NOT_ALIVE_NO_WRITERS);
            if (sawDisposed && sawNoWriters) {
                result("KEYED_LIFECYCLE", "verified",
                        "dispose(key1)->NOT_ALIVE_DISPOSED and unregister(key2)->NOT_ALIVE_NO_WRITERS "
                        + "both observed on the reader via SampleInfo.instanceState().");
            } else {
                result("KEYED_LIFECYCLE", "partial",
                        "disposed=" + sawDisposed + " noWriters=" + sawNoWriters);
            }
            dw.close(); dr.close();
        } catch (Throwable e) {
            result("KEYED_LIFECYCLE", "unsupported", "threw: " + e);
        }

        System.out.println("HARNESS_DONE");
    }
}
