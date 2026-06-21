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
    // Idiomatic C++ binding now surfaces the reader-side deadline status.
    auto s = r.requested_deadline_missed_status();
    std::cout << "  requested_deadline_missed total_count=" << s.total_count
              << " total_count_change=" << s.total_count_change << "\n";
    expect("DEADLINE: missed deadline increments requested_deadline_missed", s.total_count > 0);
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
    // Idiomatic C++ binding: PARTITION carried through PublisherQos/SubscriberQos
    // (qos_bridge.hpp now wires partition.names from the policy's string vector).
    auto trial = [&](const char* pubName, const char* subName) -> int {
        // TWO participants -> real RTPS path where partition is matched at SEDP.
        dds::domain::DomainParticipant pubDp(0);
        dds::domain::DomainParticipant subDp(0);
        dds::topic::Topic<qos::Reading> tp(pubDp, "QosPartition");
        dds::topic::Topic<qos::Reading> ts2(subDp, "QosPartition");

        dds::core::PublisherQos pq;
        if (pubName) pq.partition.name({ std::string(pubName) });
        dds::pub::Publisher pub(pubDp, pq);

        dds::core::SubscriberQos sq;
        if (subName) sq.partition.name({ std::string(subName) });
        dds::sub::Subscriber sub(subDp, sq);

        dds::core::DataWriterQos wq;
        wq.reliability = P::Reliability::Reliable_();
        wq.history = P::History::KeepLast(10);
        dds::pub::DataWriter<qos::Reading> dw(pub, tp, wq);

        dds::core::DataReaderQos rq;
        rq.reliability = P::Reliability::Reliable_();
        rq.history = P::History::KeepLast(10);
        dds::sub::DataReader<qos::Reading> dr(sub, ts2, rq);

        // Give SEDP time to (not) match across partitions; don't wait_for_matched
        // because the mismatch trial must never match.
        std::this_thread::sleep_for(400ms);
        dw.write(qos::Reading(51, 1, 5.0));

        int got = 0, waited = 0;
        while (waited < 1200) {
            auto loaned = dr.take();
            for (auto& s : loaned) if (s.info().valid_data) ++got;
            std::this_thread::sleep_for(10ms); waited += 10;
            if (got > 0) break;
        }
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
    // Idiomatic C++ binding: ContentFilteredTopic + CFT-aware DataReader
    // constructor (binds via native_cft_handle()).
    // Writer on its own participant; reader+CFT on a second participant -> real
    // RTPS path (avoids the intra-runtime shortcut, which has no CFT eval).
    dds::domain::DomainParticipant pubDp(0);
    dds::domain::DomainParticipant subDp(0);
    dds::topic::Topic<qos::Reading> tpw(pubDp, "QosCft");
    dds::topic::Topic<qos::Reading> t(subDp, "QosCft");

    // Filter: seq > 4
    std::vector<std::string> params; // literal expression, no positional params
    dds::topic::ContentFilteredTopic<qos::Reading> cft(
        subDp, "QosCftFiltered", t, "seq > 4", params);
    expect("CFT create_contentfilteredtopic succeeds", cft.native_cft_handle() != nullptr);

    dds::pub::Publisher pub(pubDp);
    dds::sub::Subscriber sub(subDp);

    dds::core::DataWriterQos wq;
    wq.reliability = P::Reliability::Reliable_();
    wq.history = P::History::KeepLast(20);
    dds::pub::DataWriter<qos::Reading> dw(pub, tpw, wq);

    dds::core::DataReaderQos rq;
    rq.reliability = P::Reliability::Reliable_();
    rq.history = P::History::KeepLast(20);
    dds::sub::DataReader<qos::Reading> dr(sub, cft, rq); // CFT-aware ctor
    expect("CFT create_datareader_with_cft succeeds", dr.native_handle() != nullptr);

    dw.wait_for_matched(1, Duration(5, 0));
    // write seq 0..9
    for (int i = 0; i < 10; ++i) dw.write(qos::Reading(61, i, i * 1.0));

    std::vector<int> seqs; int waited = 0;
    while (waited < 1500) {
        auto loaned = dr.take();
        for (auto& s : loaned) if (s.info().valid_data) seqs.push_back(s.data().seq());
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

// ------------------------------------------------------------------------
// 9. KEYED LIFECYCLE
// ------------------------------------------------------------------------
static void test_keyed_lifecycle() {
    std::cout << "\n== 9. KEYED LIFECYCLE ==\n";
    // Idiomatic C++ binding: DataWriter<T> now exposes dispose_instance(),
    // unregister_instance(), register_instance(), lookup_instance(); the typed
    // DataReader<T>::take() carries instance_state in Sample::info().

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

    // Two instances by key: id=100, id=200.
    w.write(qos::Reading(100, 0, 1.0));
    w.write(qos::Reading(200, 0, 2.0));

    auto collect = [&](int ms) {
        std::vector<std::pair<int,uint32_t>> out; // (id_if_valid_else_-1, instance_state)
        int waited = 0;
        while (waited < ms) {
            auto loaned = r.take();
            for (auto& s : loaned) {
                int id = s.info().valid_data ? s.data().id() : -1;
                out.emplace_back(id, s.info().instance_state);
            }
            std::this_thread::sleep_for(10ms); waited += 10;
        }
        return out;
    };

    auto s0 = collect(400);
    bool sawAlive = false;
    int aliveIds = 0;
    for (auto& e : s0) if (e.second & IS_ALIVE) { sawAlive = true; ++aliveIds; }
    std::cout << "  initial: " << s0.size() << " sample(s), alive markers=" << aliveIds << "\n";
    expect("KEYED: distinct instances by key are ALIVE", sawAlive);

    // Poll the reader until a given instance_state bit is observed (or timeout).
    // Accumulating across the whole drain avoids racing a single fixed window
    // against when the lifecycle DATA submessage arrives.
    auto poll_for_state = [&](uint32_t bit, int ms) -> bool {
        int waited = 0;
        while (waited < ms) {
            auto loaned = r.take();
            for (auto& s : loaned) if (s.info().instance_state & bit) return true;
            std::this_thread::sleep_for(10ms); waited += 10;
        }
        return false;
    };

    // DISPOSE key=100 via the idiomatic binding.
    w.dispose_instance(qos::Reading(100, 0, 0.0));
    std::cout << "  dispose_instance(key=100)\n";
    bool sawDisposed = poll_for_state(IS_DISPOSED, 1000);
    std::cout << "  after dispose: disposed-marker seen=" << sawDisposed << "\n";
    expect("KEYED: dispose(key) -> reader sees NOT_ALIVE_DISPOSED", sawDisposed);

    // UNREGISTER key=200 via the idiomatic binding (resolves handle from key).
    w.unregister_instance(qos::Reading(200, 0, 0.0));
    std::cout << "  unregister_instance(key=200)\n";
    bool sawNoWriters = poll_for_state(IS_NO_WRITERS, 1000);
    std::cout << "  after unregister: no-writers-marker seen=" << sawNoWriters << "\n";
    expect("KEYED: unregister(key) -> reader sees NOT_ALIVE_NO_WRITERS", sawNoWriters);

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
