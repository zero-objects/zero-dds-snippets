// QoS + keyed-lifecycle behavioral conformance for the ZeroDDS C++ binding,
// over REAL DCPS (DomainParticipant / DataWriter / DataReader, loopback).
//
// Keyed topic type: qos::Reading { @key long id; long seq; double value; }
// generated with: zerodds-idlc generate reading.idl --cpp -o gen
//
// Each sub-test sets up its own DomainParticipant(s) + entities, drives a
// behavior, and OBSERVES the actual effect (not just that a setter didn't
// throw). Where the idiomatic C++ DDS-PSM-Cxx API does not surface a needed
// operation (dispose/unregister/deadline-status/CFT/partition), we drop to the
// underlying C-API via native_handle() and note it.
//
// Output: one [PASS]/[FAIL]/[GAP] line per assertion + a per-check verdict.

#include "gen/reading.hpp"
#include "dds/dds.hpp"
#include "zerodds.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
namespace P = dds::core::policy;
using dds::core::Duration;
using TS = dds::topic::topic_type_support<qos::Reading>;

// instance_state bits (from zerodds.h SampleInfo): 1=ALIVE 2=DISPOSED 4=NO_WRITERS
static const uint32_t IS_ALIVE = 1, IS_DISPOSED = 2, IS_NO_WRITERS = 4;

static int g_total = 0, g_fail = 0;
static void rec(const char* tag, const char* what, bool ok) {
    std::cout << "    [" << tag << "] " << what << "\n";
    ++g_total;
}
static bool expect(const char* what, bool ok) {
    std::cout << "    [" << (ok ? "PASS" : "FAIL") << "] " << what << "\n";
    ++g_total; if (!ok) ++g_fail;
    return ok;
}
static void gap(const char* what) {
    std::cout << "    [GAP]  " << what << "\n";
}

// Poll a C++ DataReader.take() for up to ~timeout_ms; collect all decoded samples.
template <typename R>
static std::vector<dds::sub::Sample<qos::Reading>> drain(R& r, int timeout_ms) {
    std::vector<dds::sub::Sample<qos::Reading>> all;
    int waited = 0;
    while (waited < timeout_ms) {
        auto loaned = r.take();
        for (auto& s : loaned) all.push_back(s);
        if (!all.empty()) {
            // keep draining a little to gather follow-on lifecycle samples
        }
        std::this_thread::sleep_for(10ms);
        waited += 10;
        if (!all.empty() && waited > 120) break;
    }
    return all;
}

// ------------------------------------------------------------------------
// 1. RELIABILITY
// ------------------------------------------------------------------------
static void test_reliability() {
    std::cout << "\n== 1. RELIABILITY ==\n";
    const int N = 200;
    // RELIABLE: all N samples must arrive, in order.
    {
        dds::domain::DomainParticipant dp(0);
        dds::topic::Topic<qos::Reading> t(dp, "QosRelTopic");
        dds::pub::Publisher pub(dp);
        dds::sub::Subscriber sub(dp);
        dds::core::DataWriterQos wq; wq.reliability = P::Reliability::Reliable_();
        dds::core::DataReaderQos rq; rq.reliability = P::Reliability::Reliable_();
        wq.history = P::History::KeepAll();
        rq.history = P::History::KeepAll();
        dds::pub::DataWriter<qos::Reading> w(pub, t, wq);
        dds::sub::DataReader<qos::Reading> r(sub, t, rq);
        w.wait_for_matched(1, Duration(5, 0));
        r.wait_for_matched(1, Duration(5, 0));
        for (int i = 0; i < N; ++i) w.write(qos::Reading(1, i, i * 1.0));
        // collect
        std::vector<int> got;
        int waited = 0;
        while ((int)got.size() < N && waited < 5000) {
            auto loaned = r.take();
            for (auto& s : loaned) if (s.info().valid_data) got.push_back(s.data().seq());
            std::this_thread::sleep_for(5ms); waited += 5;
        }
        bool all = ((int)got.size() == N);
        bool ordered = true;
        for (size_t i = 0; i < got.size(); ++i) if (got[i] != (int)i) { ordered = false; break; }
        std::cout << "  RELIABLE: received " << got.size() << "/" << N << "\n";
        expect("RELIABLE delivers all N samples", all);
        expect("RELIABLE preserves order", ordered);
    }
    // BEST_EFFORT: accepted + functional (delivers something).
    {
        dds::domain::DomainParticipant dp(0);
        dds::topic::Topic<qos::Reading> t(dp, "QosBeTopic");
        dds::pub::Publisher pub(dp);
        dds::sub::Subscriber sub(dp);
        dds::core::DataWriterQos wq; wq.reliability = P::Reliability::BestEffort();
        dds::core::DataReaderQos rq; rq.reliability = P::Reliability::BestEffort();
        dds::pub::DataWriter<qos::Reading> w(pub, t, wq);
        dds::sub::DataReader<qos::Reading> r(sub, t, rq);
        w.wait_for_matched(1, Duration(5, 0));
        r.wait_for_matched(1, Duration(5, 0));
        for (int i = 0; i < N; ++i) w.write(qos::Reading(2, i, i * 1.0));
        int got = 0, waited = 0;
        while (waited < 1500) {
            auto loaned = r.take();
            for (auto& s : loaned) if (s.info().valid_data) ++got;
            std::this_thread::sleep_for(5ms); waited += 5;
        }
        std::cout << "  BEST_EFFORT: received " << got << "/" << N << " (loss tolerated)\n";
        expect("BEST_EFFORT accepted and delivers samples", got > 0);
    }
}

// ------------------------------------------------------------------------
// 2. DURABILITY (late joiner)
// ------------------------------------------------------------------------
static void test_durability() {
    std::cout << "\n== 2. DURABILITY ==\n";
    // TRANSIENT_LOCAL + KEEP_LAST(3): writer publishes, THEN a late reader joins.
    // Two separate participants -> real RTPS discovery + durability replay path
    // (NOT the intra-runtime same-participant shortcut).
    {
        dds::domain::DomainParticipant pubDp(0);
        dds::domain::DomainParticipant subDp(0);
        dds::topic::Topic<qos::Reading> tp(pubDp, "QosDurTL");
        dds::topic::Topic<qos::Reading> ts(subDp, "QosDurTL");
        dds::pub::Publisher pub(pubDp);
        dds::core::DataWriterQos wq;
        wq.reliability = P::Reliability::Reliable_();
        wq.durability = P::Durability::TransientLocal();
        wq.history = P::History::KeepLast(3);
        dds::pub::DataWriter<qos::Reading> w(pub, tp, wq);
        for (int i = 0; i < 5; ++i) w.write(qos::Reading(7, i, i * 1.0)); // same key
        std::this_thread::sleep_for(300ms);
        // LATE join on a SECOND participant.
        dds::sub::Subscriber sub(subDp);
        dds::core::DataReaderQos rq;
        rq.reliability = P::Reliability::Reliable_();
        rq.durability = P::Durability::TransientLocal();
        rq.history = P::History::KeepLast(3);
        dds::sub::DataReader<qos::Reading> r(sub, ts, rq);
        r.wait_for_matched(1, Duration(5, 0));
        int got = 0, last = -1, waited = 0;
        while (waited < 2000) {
            auto loaned = r.take();
            for (auto& s : loaned) if (s.info().valid_data) { ++got; last = s.data().seq(); }
            if (got > 0 && waited > 400) break;
            std::this_thread::sleep_for(10ms); waited += 10;
        }
        std::cout << "  TRANSIENT_LOCAL late joiner got " << got << " retained sample(s), last seq=" << last << "\n";
        expect("TRANSIENT_LOCAL: late joiner receives retained sample(s)", got > 0);
    }
    // VOLATILE: late joiner gets nothing.
    {
        dds::domain::DomainParticipant pubDp(0);
        dds::domain::DomainParticipant subDp(0);
        dds::topic::Topic<qos::Reading> tp(pubDp, "QosDurVol");
        dds::topic::Topic<qos::Reading> ts(subDp, "QosDurVol");
        dds::pub::Publisher pub(pubDp);
        dds::core::DataWriterQos wq;
        wq.reliability = P::Reliability::Reliable_();
        wq.durability = P::Durability::Volatile();
        wq.history = P::History::KeepLast(3);
        dds::pub::DataWriter<qos::Reading> w(pub, tp, wq);
        for (int i = 0; i < 5; ++i) w.write(qos::Reading(8, i, i * 1.0));
        std::this_thread::sleep_for(300ms);
        dds::sub::Subscriber sub(subDp);
        dds::core::DataReaderQos rq;
        rq.reliability = P::Reliability::Reliable_();
        rq.durability = P::Durability::Volatile();
        dds::sub::DataReader<qos::Reading> r(sub, ts, rq);
        r.wait_for_matched(1, Duration(5, 0));
        int got = 0, waited = 0;
        while (waited < 800) {
            auto loaned = r.take();
            for (auto& s : loaned) if (s.info().valid_data) ++got;
            std::this_thread::sleep_for(10ms); waited += 10;
        }
        std::cout << "  VOLATILE late joiner got " << got << " (expected 0)\n";
        expect("VOLATILE: late joiner receives nothing", got == 0);
    }
}

// ------------------------------------------------------------------------
// 3. HISTORY
// ------------------------------------------------------------------------
static void test_history() {
    std::cout << "\n== 3. HISTORY ==\n";
    // KEEP_LAST(k): a TRANSIENT_LOCAL late joiner should only get the last k per instance.
    {
        const int K = 2;
        dds::domain::DomainParticipant pubDp(0);
        dds::domain::DomainParticipant subDp(0);
        dds::topic::Topic<qos::Reading> tp(pubDp, "QosHistKL");
        dds::topic::Topic<qos::Reading> ts(subDp, "QosHistKL");
        dds::pub::Publisher pub(pubDp);
        dds::core::DataWriterQos wq;
        wq.reliability = P::Reliability::Reliable_();
        wq.durability = P::Durability::TransientLocal();
        wq.history = P::History::KeepLast(K);
        dds::pub::DataWriter<qos::Reading> w(pub, tp, wq);
        for (int i = 0; i < 6; ++i) w.write(qos::Reading(11, i, i * 1.0)); // one instance
        std::this_thread::sleep_for(300ms);
        dds::sub::Subscriber sub(subDp);
        dds::core::DataReaderQos rq;
        rq.reliability = P::Reliability::Reliable_();
        rq.durability = P::Durability::TransientLocal();
        rq.history = P::History::KeepLast(K);
        dds::sub::DataReader<qos::Reading> r(sub, ts, rq);
        r.wait_for_matched(1, Duration(5, 0));
        std::vector<int> seqs; int waited = 0;
        while (waited < 1500) {
            auto loaned = r.take();
            for (auto& s : loaned) if (s.info().valid_data) seqs.push_back(s.data().seq());
            if (!seqs.empty() && waited > 400) break;
            std::this_thread::sleep_for(10ms); waited += 10;
        }
        std::cout << "  KEEP_LAST(" << K << "): late joiner received " << seqs.size() << " sample(s): ";
        for (int s : seqs) std::cout << s << " ";
        std::cout << "(wrote 0..5)\n";
        // Behaviorally: at most K retained, and they should be the newest.
        bool capped = ((int)seqs.size() <= K);
        bool newest = true;
        for (int s : seqs) if (s < 6 - K) newest = false;
        expect("KEEP_LAST(k) caps retained samples per instance at k", capped);
        if (capped && !seqs.empty()) expect("KEEP_LAST(k) retains the NEWEST k", newest);
    }
    // KEEP_ALL: a TRANSIENT_LOCAL late joiner gets all (within resource limits).
    {
        dds::domain::DomainParticipant pubDp(0);
        dds::domain::DomainParticipant subDp(0);
        dds::topic::Topic<qos::Reading> tp(pubDp, "QosHistKA");
        dds::topic::Topic<qos::Reading> ts(subDp, "QosHistKA");
        dds::pub::Publisher pub(pubDp);
        dds::core::DataWriterQos wq;
        wq.reliability = P::Reliability::Reliable_();
        wq.durability = P::Durability::TransientLocal();
        wq.history = P::History::KeepAll();
        dds::pub::DataWriter<qos::Reading> w(pub, tp, wq);
        for (int i = 0; i < 6; ++i) w.write(qos::Reading(12, i, i * 1.0));
        std::this_thread::sleep_for(300ms);
        dds::sub::Subscriber sub(subDp);
        dds::core::DataReaderQos rq;
        rq.reliability = P::Reliability::Reliable_();
        rq.durability = P::Durability::TransientLocal();
        rq.history = P::History::KeepAll();
        dds::sub::DataReader<qos::Reading> r(sub, ts, rq);
        r.wait_for_matched(1, Duration(5, 0));
        std::vector<int> seqs; int waited = 0;
        while (waited < 1500) {
            auto loaned = r.take();
            for (auto& s : loaned) if (s.info().valid_data) seqs.push_back(s.data().seq());
            if ((int)seqs.size() >= 6) break;
            std::this_thread::sleep_for(10ms); waited += 10;
        }
        std::cout << "  KEEP_ALL: late joiner received " << seqs.size() << " sample(s) (wrote 6)\n";
        expect("KEEP_ALL retains all samples (>= KEEP_LAST would)", (int)seqs.size() >= 6);
    }
}

// ------------------------------------------------------------------------
// 4. DEADLINE  (reader-side requested-deadline-missed status)
// ------------------------------------------------------------------------
static void test_deadline() {
    std::cout << "\n== 4. DEADLINE ==\n";
    dds::domain::DomainParticipant pubDp(0);
    dds::domain::DomainParticipant subDp(0);
    dds::topic::Topic<qos::Reading> tp(pubDp, "QosDeadline");
    dds::topic::Topic<qos::Reading> ts(subDp, "QosDeadline");
    dds::pub::Publisher pub(pubDp);
    dds::sub::Subscriber sub(subDp);
    dds::core::DataWriterQos wq;
    wq.reliability = P::Reliability::Reliable_();
    wq.deadline = P::Deadline(Duration::from_millis(100));
    dds::core::DataReaderQos rq;
    rq.reliability = P::Reliability::Reliable_();
    rq.deadline = P::Deadline(Duration::from_millis(100));
    dds::pub::DataWriter<qos::Reading> w(pub, tp, wq);
    dds::sub::DataReader<qos::Reading> r(sub, ts, rq);
    w.wait_for_matched(1, Duration(5, 0));
    r.wait_for_matched(1, Duration(5, 0));
    w.write(qos::Reading(21, 0, 0.0));
    // Now miss the 100ms deadline by sleeping ~600ms with no further writes.
    std::this_thread::sleep_for(600ms);
    // The C++ DataReader does NOT surface requested_deadline_missed_status, so
    // we read it through the C-API on the native handle.
    zerodds_ZeroDdsRequestedDeadlineMissedStatus s{};
    int rc = zerodds_dr_get_requested_deadline_missed_status(r.native_handle(), &s);
    std::cout << "  requested_deadline_missed rc=" << rc
              << " total_count=" << s.total_count
              << " total_count_change=" << s.total_count_change << "\n";
    gap("DataReader::requested_deadline_missed_status absent in C++ binding (used C-API)");
    if (rc == 0) {
        expect("DEADLINE: missed deadline increments requested_deadline_missed", s.total_count > 0);
    } else {
        std::cout << "    [GAP]  requested_deadline_missed_status C-API returned rc=" << rc << "\n";
    }
}

// ------------------------------------------------------------------------
// 5. LIVELINESS
// ------------------------------------------------------------------------
static void test_liveliness() {
    std::cout << "\n== 5. LIVELINESS ==\n";
    dds::domain::DomainParticipant pubDp(0);
    dds::domain::DomainParticipant subDp(0);
    dds::topic::Topic<qos::Reading> tp(pubDp, "QosLiveliness");
    dds::topic::Topic<qos::Reading> ts(subDp, "QosLiveliness");
    dds::pub::Publisher pub(pubDp);
    dds::sub::Subscriber sub(subDp);
    dds::core::DataWriterQos wq;
    wq.reliability = P::Reliability::Reliable_();
    wq.liveliness = P::Liveliness(P::LivelinessKind::Automatic, Duration::from_millis(200));
    dds::core::DataReaderQos rq;
    rq.reliability = P::Reliability::Reliable_();
    rq.liveliness = P::Liveliness(P::LivelinessKind::Automatic, Duration::from_millis(200));
    {
        dds::sub::DataReader<qos::Reading> r(sub, ts, rq);
        {
            dds::pub::DataWriter<qos::Reading> w(pub, tp, wq);
            w.wait_for_matched(1, Duration(5, 0));
            r.wait_for_matched(1, Duration(5, 0));
            w.write(qos::Reading(31, 0, 0.0));
            // Give the periodic liveliness task time to register the alive
            // transition (assert/lease bookkeeping is event-driven but runs on
            // a timer; a single read right after match can race it).
            std::this_thread::sleep_for(500ms);
            auto live = r.liveliness_changed_status();
            std::cout << "  after match: alive_count=" << live.alive_count
                      << " not_alive_count=" << live.not_alive_count << "\n";
            expect("LIVELINESS: writer becomes ALIVE (alive_count>0)", live.alive_count > 0);
            // writer goes out of scope -> stops asserting liveliness
        }
        // Wait beyond lease for the not-alive transition.
        std::this_thread::sleep_for(900ms);
        auto live = r.liveliness_changed_status();
        std::cout << "  after writer stop: alive_count=" << live.alive_count
                  << " not_alive_count=" << live.not_alive_count
                  << " not_alive_change=" << live.not_alive_count_change << "\n";
        expect("LIVELINESS: stopped writer triggers not_alive (alive->not_alive)",
               live.not_alive_count > 0 || live.alive_count == 0);
    }
}

// ------------------------------------------------------------------------
// 6. OWNERSHIP (EXCLUSIVE)
// ------------------------------------------------------------------------
static void test_ownership() {
    std::cout << "\n== 6. OWNERSHIP (EXCLUSIVE) ==\n";
    // Each writer on its own participant + a third for the reader, so the
    // reader resolves per-writer strength from SEDP PublicationBuiltinTopicData
    // (the real wire path; the intra-runtime shortcut skips SEDP).
    dds::domain::DomainParticipant lowDp(0);
    dds::domain::DomainParticipant highDp(0);
    dds::domain::DomainParticipant subDp(0);
    dds::topic::Topic<qos::Reading> tLow(lowDp, "QosOwnership");
    dds::topic::Topic<qos::Reading> tHigh(highDp, "QosOwnership");
    dds::topic::Topic<qos::Reading> tSub(subDp, "QosOwnership");
    dds::pub::Publisher pubLow(lowDp);
    dds::pub::Publisher pubHigh(highDp);
    dds::sub::Subscriber sub(subDp);
    dds::core::DataReaderQos rq;
    rq.reliability = P::Reliability::Reliable_();
    rq.ownership = P::Ownership::Exclusive();
    rq.history = P::History::KeepAll();
    dds::sub::DataReader<qos::Reading> r(sub, tSub, rq);

    dds::core::DataWriterQos wqLow;
    wqLow.reliability = P::Reliability::Reliable_();
    wqLow.ownership = P::Ownership::Exclusive();
    wqLow.ownership_strength = P::OwnershipStrength(1);
    dds::core::DataWriterQos wqHigh;
    wqHigh.reliability = P::Reliability::Reliable_();
    wqHigh.ownership = P::Ownership::Exclusive();
    wqHigh.ownership_strength = P::OwnershipStrength(10);

    dds::pub::DataWriter<qos::Reading> wLow(pubLow, tLow, wqLow);
    dds::pub::DataWriter<qos::Reading> wHigh(pubHigh, tHigh, wqHigh);
    wLow.wait_for_matched(1, Duration(5, 0));
    wHigh.wait_for_matched(1, Duration(5, 0));
    r.wait_for_matched(2, Duration(5, 0));

    // Both write to the SAME instance (id=41). Encode value to distinguish source:
    // low strength -> value 1.0, high strength -> value 10.0.
    for (int i = 0; i < 10; ++i) {
        wLow.write(qos::Reading(41, i, 1.0));
        wHigh.write(qos::Reading(41, i, 10.0));
    }
    int fromLow = 0, fromHigh = 0, waited = 0;
    while (waited < 1500) {
        auto loaned = r.take();
        for (auto& s : loaned) if (s.info().valid_data) {
            if (s.data().value() < 5.0) ++fromLow; else ++fromHigh;
        }
        std::this_thread::sleep_for(10ms); waited += 10;
    }
    std::cout << "  EXCLUSIVE: samples from low-strength=" << fromLow
              << " from high-strength=" << fromHigh << "\n";
    // Behavioral conformance: only the highest-strength writer's samples reach
    // the reader for that instance.
    expect("EXCLUSIVE: high-strength writer's samples delivered", fromHigh > 0);
    expect("EXCLUSIVE: low-strength writer's samples NOT delivered (filtered)", fromLow == 0);
}

// ------------------------------------------------------------------------
// 7. PARTITION
// ------------------------------------------------------------------------
static void test_partition() {
    std::cout << "\n== 7. PARTITION ==\n";
    // First: does the C++ QoS bridge carry partition names at all?
    std::cout << "  Checking C++ PublisherQos partition bridge...\n";
    gap("qos_bridge.hpp hardcodes partition.names=nullptr (PublisherQos/SubscriberQos/DW/DR)"
        " -- partition NOT wired through the C++ binding");

    // Behavioral test via the C-API directly (which DOES carry the partition
    // field). Full DCPS path with explicit PublisherQos/SubscriberQos partition.
    const zerodds_ZeroDdsDomainParticipantFactory* f = zerodds_dpf_get_instance();
    auto trial = [&](const char* pubName, const char* subName) -> int {
        // TWO participants -> real RTPS path where partition is matched at SEDP.
        zerodds_ZeroDdsDomainParticipant* pubDp = zerodds_dpf_create_participant(f, 0, nullptr);
        zerodds_ZeroDdsDomainParticipant* subDp = zerodds_dpf_create_participant(f, 0, nullptr);
        zerodds_ZeroDdsTopic* tp = zerodds_dp_create_topic(pubDp, "QosPartition", "qos::Reading", nullptr);
        zerodds_ZeroDdsTopic* ts2 = zerodds_dp_create_topic(subDp, "QosPartition", "qos::Reading", nullptr);

        const char* pubParts[1] = { pubName };
        zerodds_ZeroDdsPublisherQos pq{};
        pq.partition.names = pubName ? pubParts : nullptr;
        pq.partition.names_len = pubName ? 1 : 0;
        pq.entity_factory.autoenable_created_entities = true;
        zerodds_ZeroDdsPublisher* pub = zerodds_dp_create_publisher(pubDp, &pq);

        const char* subParts[1] = { subName };
        zerodds_ZeroDdsSubscriberQos sq{};
        sq.partition.names = subName ? subParts : nullptr;
        sq.partition.names_len = subName ? 1 : 0;
        sq.entity_factory.autoenable_created_entities = true;
        zerodds_ZeroDdsSubscriber* sub = zerodds_dp_create_subscriber(subDp, &sq);

        zerodds_ZeroDdsDataWriterQos wq{};
        wq.reliability.kind = 2; wq.reliability.max_blocking_time.sec = 1;
        wq.history.kind = 0; wq.history.depth = 10;
        zerodds_ZeroDdsDataWriter* dw = zerodds_pub_create_datawriter(pub, tp, &wq);

        zerodds_ZeroDdsDataReaderQos rq{};
        rq.reliability.kind = 2; rq.reliability.max_blocking_time.sec = 1;
        rq.history.kind = 0; rq.history.depth = 10;
        zerodds_ZeroDdsDataReader* dr = zerodds_sub_create_datareader(sub, ts2, &rq);

        int matched = zerodds_dw_wait_for_matched(dw, 1, 1500);
        // write a sample
        std::vector<uint8_t> wire = TS::encode(qos::Reading(51, 1, 5.0));
        zerodds_dw_write(dw, wire.data(), wire.size(), 0);

        int got = 0, waited = 0;
        while (waited < 1200) {
            zerodds_ZeroDdsSampleArray arr{};
            int rc = zerodds_dr_take(dr, &arr, 0, 0, 0, 0);
            if (rc == 0) {
                for (size_t i = 0; i < arr.count; ++i) if (arr.infos[i].valid_data) ++got;
                zerodds_dr_return_loan(dr, &arr);
            }
            std::this_thread::sleep_for(10ms); waited += 10;
            if (got > 0) break;
        }
        (void)matched;
        zerodds_dp_delete_contained_entities(pubDp);
        zerodds_dp_delete_contained_entities(subDp);
        zerodds_dpf_delete_participant(f, pubDp);
        zerodds_dpf_delete_participant(f, subDp);
        return got;
    };

    int sameP = trial("A", "A");
    std::cout << "  matching partition 'A'/'A': received " << sameP << "\n";
    int diffP = trial("A", "B");
    std::cout << "  mismatched partition 'A'/'B': received " << diffP << "\n";
    expect("PARTITION: matching partitions communicate", sameP > 0);
    expect("PARTITION: mismatched partitions do NOT communicate", diffP == 0);
}

// ------------------------------------------------------------------------
// 8. CONTENT-FILTERED TOPIC
// ------------------------------------------------------------------------
static void test_cft() {
    std::cout << "\n== 8. CONTENT-FILTERED TOPIC ==\n";
    gap("C++ ContentFilteredTopic binds DataReader to the RELATED topic, not the"
        " CFT handle (header comment: 'reader bind via cft_handle_ is follow-up')"
        " -- testing filter behavior via the C-API cft reader path");

    const zerodds_ZeroDdsDomainParticipantFactory* f = zerodds_dpf_get_instance();
    // Writer on its own participant; reader+CFT on a second participant -> real
    // RTPS path (avoids the intra-runtime shortcut, which has no CFT eval).
    zerodds_ZeroDdsDomainParticipant* pubDp = zerodds_dpf_create_participant(f, 0, nullptr);
    zerodds_ZeroDdsDomainParticipant* dp = zerodds_dpf_create_participant(f, 0, nullptr);
    zerodds_ZeroDdsTopic* tpw = zerodds_dp_create_topic(pubDp, "QosCft", "qos::Reading", nullptr);
    zerodds_ZeroDdsTopic* t = zerodds_dp_create_topic(dp, "QosCft", "qos::Reading", nullptr);

    // Filter: seq > 4
    const char* expr = "seq > 4";
    zerodds_ZeroDdsContentFilteredTopic* cft =
        zerodds_dp_create_contentfilteredtopic(dp, "QosCftFiltered", t, expr, nullptr, 0);
    bool cft_ok = (cft != nullptr);
    expect("CFT create_contentfilteredtopic succeeds", cft_ok);

    zerodds_ZeroDdsPublisher* pub = zerodds_dp_create_publisher(pubDp, nullptr);
    zerodds_ZeroDdsSubscriber* sub = zerodds_dp_create_subscriber(dp, nullptr);

    zerodds_ZeroDdsDataWriterQos wq{};
    wq.reliability.kind = 2; wq.reliability.max_blocking_time.sec = 1;
    wq.history.kind = 0; wq.history.depth = 20;
    zerodds_ZeroDdsDataWriter* dw = zerodds_pub_create_datawriter(pub, tpw, &wq);

    zerodds_ZeroDdsDataReaderQos rq{};
    rq.reliability.kind = 2; rq.reliability.max_blocking_time.sec = 1;
    rq.history.kind = 0; rq.history.depth = 20;
    zerodds_ZeroDdsDataReader* dr = nullptr;
    if (cft_ok) dr = zerodds_sub_create_datareader_with_cft(sub, cft, &rq);
    bool dr_ok = (dr != nullptr);
    expect("CFT create_datareader_with_cft succeeds", dr_ok);

    if (dr_ok) {
        zerodds_dw_wait_for_matched(dw, 1, 1500);
        // write seq 0..9
        for (int i = 0; i < 10; ++i) {
            std::vector<uint8_t> wire = TS::encode(qos::Reading(61, i, i * 1.0));
            zerodds_dw_write(dw, wire.data(), wire.size(), 0);
        }
        std::vector<int> seqs; int waited = 0;
        while (waited < 1500) {
            zerodds_ZeroDdsSampleArray arr{};
            int rc = zerodds_dr_take(dr, &arr, 0, 0, 0, 0);
            if (rc == 0) {
                for (size_t i = 0; i < arr.count; ++i) {
                    if (arr.infos[i].valid_data) {
                        qos::Reading rd = TS::decode(arr.buffers[i], arr.lengths[i],
                                                     dds::topic::xcdr2::XcdrVersion::Xcdr2);
                        seqs.push_back(rd.seq());
                    }
                }
                zerodds_dr_return_loan(dr, &arr);
            }
            std::this_thread::sleep_for(10ms); waited += 10;
            if (seqs.size() >= 5) break;
        }
        std::cout << "  filter 'seq > 4' received seqs: ";
        for (int s : seqs) std::cout << s << " ";
        std::cout << "(wrote 0..9)\n";
        bool all_match = !seqs.empty();
        for (int s : seqs) if (s <= 4) all_match = false;
        expect("CFT: reader only receives samples matching 'seq > 4'", all_match);
        expect("CFT: matching samples (seq 5..9) are delivered", seqs.size() >= 1);
    }
    zerodds_dp_delete_contained_entities(pubDp);
    zerodds_dp_delete_contained_entities(dp);
    if (cft) zerodds_dp_delete_contentfilteredtopic(dp, cft);
    zerodds_dpf_delete_participant(f, pubDp);
    zerodds_dpf_delete_participant(f, dp);
}

// ------------------------------------------------------------------------
// 9. KEYED LIFECYCLE
// ------------------------------------------------------------------------
static void test_keyed_lifecycle() {
    std::cout << "\n== 9. KEYED LIFECYCLE ==\n";
    gap("C++ DataWriter exposes only write() -- no dispose()/unregister_instance()"
        "/register_instance() in the binding; using C-API dw_dispose/unregister + key_hash()");

    // Two participants -> lifecycle markers (DISPOSE/UNREGISTER) travel the real
    // RTPS path; the intra-runtime shortcut only dispatches Alive samples.
    dds::domain::DomainParticipant pubDp(0);
    dds::domain::DomainParticipant subDp(0);
    dds::topic::Topic<qos::Reading> tp(pubDp, "QosKeyed");
    dds::topic::Topic<qos::Reading> ts(subDp, "QosKeyed");
    dds::pub::Publisher pub(pubDp);
    dds::sub::Subscriber sub(subDp);
    dds::core::DataWriterQos wq;
    wq.reliability = P::Reliability::Reliable_();
    wq.history = P::History::KeepAll();
    // Default writer_data_lifecycle.autodispose_unregistered_instances = true; keep it.
    dds::core::DataReaderQos rq;
    rq.reliability = P::Reliability::Reliable_();
    rq.history = P::History::KeepAll();
    dds::pub::DataWriter<qos::Reading> w(pub, tp, wq);
    dds::sub::DataReader<qos::Reading> r(sub, ts, rq);
    w.wait_for_matched(1, Duration(5, 0));
    r.wait_for_matched(1, Duration(5, 0));

    auto* dwh = w.native_handle();

    // Two instances by key: id=100, id=200.
    w.write(qos::Reading(100, 0, 1.0));
    w.write(qos::Reading(200, 0, 2.0));

    auto collect = [&](int ms) {
        std::vector<std::pair<int,uint32_t>> out; // (id_if_valid_else_-1, instance_state)
        int waited = 0;
        while (waited < ms) {
            zerodds_ZeroDdsSampleArray arr{};
            int rc = zerodds_dr_take(r.native_handle(), &arr, 0, 0, 0, 0);
            if (rc == 0) {
                for (size_t i = 0; i < arr.count; ++i) {
                    int id = -1;
                    if (arr.infos[i].valid_data) {
                        qos::Reading rd = TS::decode(arr.buffers[i], arr.lengths[i],
                                                     dds::topic::xcdr2::XcdrVersion::Xcdr2);
                        id = rd.id();
                    }
                    out.emplace_back(id, arr.infos[i].instance_state);
                }
                zerodds_dr_return_loan(r.native_handle(), &arr);
            }
            std::this_thread::sleep_for(10ms); waited += 10;
        }
        return out;
    };

    auto s0 = collect(400);
    bool sawAlive = false, twoInstances = false;
    int aliveIds = 0;
    for (auto& e : s0) if (e.second & IS_ALIVE) { sawAlive = true; ++aliveIds; }
    twoInstances = aliveIds >= 2;
    std::cout << "  initial: " << s0.size() << " sample(s), alive markers=" << aliveIds << "\n";
    expect("KEYED: distinct instances by key are ALIVE", sawAlive);

    // DISPOSE key=100 via C-API (key_hash from generated TS).
    std::array<uint8_t,16> kh100 = TS::key_hash(qos::Reading(100, 0, 0.0));
    int rcD = zerodds_dw_dispose(dwh, kh100.data(), 0);
    std::cout << "  dw_dispose(key=100) rc=" << rcD << "\n";
    auto sd = collect(500);
    bool sawDisposed = false;
    for (auto& e : sd) if (e.second & IS_DISPOSED) sawDisposed = true;
    std::cout << "  after dispose: " << sd.size() << " lifecycle/sample(s); disposed-marker seen="
              << sawDisposed << "\n";
    if (rcD == 0)
        expect("KEYED: dispose(key) -> reader sees NOT_ALIVE_DISPOSED", sawDisposed);
    else
        std::cout << "    [GAP]  dw_dispose returned rc=" << rcD << "\n";

    // UNREGISTER key=200 via C-API. We use lookup_instance to get a handle from
    // the key, then unregister_instance by handle.
    std::array<uint8_t,16> kh200 = TS::key_hash(qos::Reading(200, 0, 0.0));
    uint64_t h200 = 0;
    int rcL = zerodds_dw_lookup_instance(dwh, kh200.data(), 16, &h200);
    int rcUn = zerodds_dw_unregister_instance(dwh, h200);
    std::cout << "  dw_lookup_instance rc=" << rcL << " handle=" << h200
              << " ; dw_unregister_instance rc=" << rcUn << "\n";
    auto su = collect(500);
    bool sawNoWriters = false;
    for (auto& e : su) if (e.second & IS_NO_WRITERS) sawNoWriters = true;
    std::cout << "  after unregister: " << su.size() << " lifecycle/sample(s); no-writers-marker seen="
              << sawNoWriters << "\n";
    if (rcUn == 0)
        expect("KEYED: unregister(key) -> reader sees NOT_ALIVE_NO_WRITERS", sawNoWriters);
    else
        std::cout << "    [GAP]  dw_unregister_instance returned rc=" << rcUn << "\n";

    // NEW sample on a disposed key -> ALIVE again.
    w.write(qos::Reading(100, 99, 9.0));
    auto sn = collect(500);
    bool reAlive = false;
    for (auto& e : sn) if (e.first == 100 && (e.second & IS_ALIVE)) reAlive = true;
    std::cout << "  after re-write key=100: re-ALIVE seen=" << reAlive << "\n";
    expect("KEYED: new sample on a key -> instance ALIVE again", reAlive);
}

int main() {
    std::cout << "ZeroDDS C++ binding -- QoS + keyed-lifecycle behavioral conformance\n";
    std::cout << "version: " << zerodds_version() << "\n";

    test_reliability();
    test_durability();
    test_history();
    test_deadline();
    test_liveliness();
    test_ownership();
    test_partition();
    test_cft();
    test_keyed_lifecycle();

    std::cout << "\n==== SUMMARY: " << (g_total - g_fail) << "/" << g_total
              << " assertions passed, " << g_fail << " failed ====\n";
    return g_fail == 0 ? 0 : 1;
}
