// SPDX-License-Identifier: Apache-2.0
//
// Fast DDS secure XTypes interop peer. Carries one XTypes-heavy topic
// (mutable / appendable / keyed / nested) over a DDS-Security session and
// proves Fast DDS decode/encode interop with ZeroDDS under encryption.
//
// fastddsgen generates the typed PubSubType (and its XCDR2 mutable/appendable
// framing). This app only registers the type, applies the same property-QoS
// security as the roundtrip-bench, and echoes (pong) or originates+verifies
// (ping). Field-content verification of the *encrypted* round-trip is performed
// on whichever side is ZeroDDS (zerodds_secxt fingerprint) AND here for the
// fastdds-ping seq/content check.
//
// Build: g++ -std=c++17 fastdds_secxt.cpp secure_xtypes.cxx
//        secure_xtypesPubSubTypes.cxx secure_xtypesTypeObjectSupport.cxx
//        -lfastdds -lfastcdr -lpthread

#include "secure_xtypes.hpp"
#include "secure_xtypesPubSubTypes.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace eprosima::fastdds::dds;

namespace {

uint32_t resolve_domain() {
    if (const char* s = std::getenv("ZERODDS_BENCH_DOMAIN")) try { return (uint32_t)std::stoul(s); } catch (...) {}
    return 200;
}

void apply_security(DomainParticipantQos& pqos) {
    const char* sec = std::getenv("ZERODDS_BENCH_SECURITY");
    if (!sec || std::string(sec) != "1") return;
    const char* sec_dir = std::getenv("ZERODDS_BENCH_SEC_DIR");
    if (!sec_dir) sec_dir = "/root/bench-security";
    const char* who = std::getenv("ZERODDS_BENCH_SEC_NAME");
    if (!who) who = "ping";
    auto& props = pqos.properties().properties();
    auto add = [&](const std::string& k, const std::string& v) { props.emplace_back(k, v); };
    add("dds.sec.auth.plugin", "builtin.PKI-DH");
    add("dds.sec.auth.builtin.PKI-DH.identity_ca", std::string("file://") + sec_dir + "/certs/identity_ca.pem");
    add("dds.sec.auth.builtin.PKI-DH.identity_certificate", std::string("file://") + sec_dir + "/certs/" + who + "_cert.pem");
    add("dds.sec.auth.builtin.PKI-DH.private_key", std::string("file://") + sec_dir + "/certs/" + who + "_key.pem");
    add("dds.sec.access.plugin", "builtin.Access-Permissions");
    add("dds.sec.access.builtin.Access-Permissions.permissions_ca", std::string("file://") + sec_dir + "/certs/permissions_ca.pem");
    add("dds.sec.access.builtin.Access-Permissions.governance", std::string("file://") + sec_dir + "/governance.p7s");
    add("dds.sec.access.builtin.Access-Permissions.permissions", std::string("file://") + sec_dir + "/permissions_" + who + ".p7s");
    add("dds.sec.crypto.plugin", "builtin.AES-GCM-GMAC");
}

const char* kReq = "SecXT_Request";
const char* kEcho = "SecXT_Echo";

void qos_xcdr2(DataWriterQos& q) {
    q.reliability().kind = RELIABLE_RELIABILITY_QOS;
    q.history().kind = KEEP_LAST_HISTORY_QOS; q.history().depth = 64;
    q.representation().m_value.clear();
    q.representation().m_value.push_back(XCDR2_DATA_REPRESENTATION);
}
void qos_xcdr2(DataReaderQos& q) {
    q.reliability().kind = RELIABLE_RELIABILITY_QOS;
    q.history().kind = KEEP_LAST_HISTORY_QOS; q.history().depth = 64;
    q.representation().m_value.clear();
    q.representation().m_value.push_back(XCDR2_DATA_REPRESENTATION);
}

// ---- Generic pong (echo) + ping templated over the typed sample T ----
template <typename T>
uint32_t seq_of(const T& m);  // specialized below

template <> uint32_t seq_of<secxt::MutableMsg>(const secxt::MutableMsg& m) { return m.seq(); }
template <> uint32_t seq_of<secxt::AppendableMsg>(const secxt::AppendableMsg& m) { return m.seq(); }
template <> uint32_t seq_of<secxt::KeyedMsg>(const secxt::KeyedMsg& m) { return m.seq(); }
template <> uint32_t seq_of<secxt::NestedMsg>(const secxt::NestedMsg& m) { return m.seq(); }

// Build deterministic sample by seq -- identical content rules to zerodds_secxt.
secxt::MutableMsg build(secxt::MutableMsg*, uint32_t seq) {
    secxt::MutableMsg m; m.seq(seq); m.a((int32_t)seq * 7 - 3); m.b(3.14159 * seq);
    m.label("mut-" + std::to_string(seq)); m.payload(std::vector<uint8_t>(64, (uint8_t)(seq & 0xFF))); return m;
}
secxt::AppendableMsg build(secxt::AppendableMsg*, uint32_t seq) {
    secxt::AppendableMsg m; m.seq(seq); m.t_send_ns(seq); m.label("app-" + std::to_string(seq));
    m.payload(std::vector<uint8_t>(64, (uint8_t)(seq & 0xFF))); return m;
}
secxt::KeyedMsg build(secxt::KeyedMsg*, uint32_t seq) {
    secxt::KeyedMsg m; m.region((int32_t)(seq % 4)); m.unit((int32_t)seq); m.seq(seq);
    m.payload(std::vector<uint8_t>(64, (uint8_t)(seq & 0xFF))); return m;
}
secxt::NestedMsg build(secxt::NestedMsg*, uint32_t seq) {
    secxt::NestedMsg m; m.seq(seq);
    secxt::Inner h; h.x((int32_t)seq); h.y(2.71 * seq); h.tag("head-" + std::to_string(seq)); m.head(h);
    std::vector<secxt::Inner> items;
    for (int i = 0; i < 3; ++i) { secxt::Inner it; it.x((int32_t)(seq * 10 + i)); it.y((double)i); it.tag("it" + std::to_string(i)); items.push_back(it); }
    m.items(items); m.payload(std::vector<uint8_t>(64, (uint8_t)(seq & 0xFF))); return m;
}

template <typename T>
bool content_eq(const T& a, const T& b);
template <> bool content_eq<secxt::MutableMsg>(const secxt::MutableMsg& a, const secxt::MutableMsg& b) {
    return a.seq()==b.seq() && a.a()==b.a() && a.label()==b.label() && a.payload()==b.payload(); }
template <> bool content_eq<secxt::AppendableMsg>(const secxt::AppendableMsg& a, const secxt::AppendableMsg& b) {
    return a.seq()==b.seq() && a.t_send_ns()==b.t_send_ns() && a.label()==b.label() && a.payload()==b.payload(); }
template <> bool content_eq<secxt::KeyedMsg>(const secxt::KeyedMsg& a, const secxt::KeyedMsg& b) {
    return a.seq()==b.seq() && a.region()==b.region() && a.unit()==b.unit() && a.payload()==b.payload(); }
template <> bool content_eq<secxt::NestedMsg>(const secxt::NestedMsg& a, const secxt::NestedMsg& b) {
    if (!(a.seq()==b.seq() && a.head().x()==b.head().x() && a.head().tag()==b.head().tag() && a.items().size()==b.items().size())) return false;
    for (size_t i=0;i<a.items().size();++i) if (!(a.items()[i].x()==b.items()[i].x() && a.items()[i].tag()==b.items()[i].tag())) return false;
    return a.payload()==b.payload(); }

template <typename T, typename PubSub>
class EchoListener : public DataReaderListener {
public:
    explicit EchoListener(DataWriter* w) : w_(w) {}
    void on_data_available(DataReader* r) override {
        T m; SampleInfo info;
        while (r->take_next_sample(&m, &info) == RETCODE_OK)
            if (info.valid_data) { w_->write(&m); ++echoed; }
    }
    std::atomic<uint64_t> echoed{0};
private:
    DataWriter* w_;
};

template <typename T, typename PubSub>
int run_pong(uint64_t secs) {
    DomainParticipantQos pqos; apply_security(pqos);
    auto* dp = DomainParticipantFactory::get_instance()->create_participant(resolve_domain(), pqos);
    if (!dp) { std::cerr << "dp create failed\n"; return 1; }
    TypeSupport ts(new PubSub()); ts.register_type(dp);
    auto* tr = dp->create_topic(kReq, ts.get_type_name(), TOPIC_QOS_DEFAULT);
    auto* te = dp->create_topic(kEcho, ts.get_type_name(), TOPIC_QOS_DEFAULT);
    auto* pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT);
    auto* sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    DataWriterQos wq; qos_xcdr2(wq); auto* w = pub->create_datawriter(te, wq);
    DataReaderQos rq; qos_xcdr2(rq);
    EchoListener<T, PubSub> lis(w);
    auto* rd = sub->create_datareader(tr, rq, &lis);
    if (!w || !rd) { std::cerr << "pong ep create failed\n"; return 1; }
    std::cout << "pong[fastdds]: matched type=" << ts.get_type_name() << "\n" << std::flush;
    std::this_thread::sleep_for(std::chrono::seconds(secs));
    std::cout << "pong[fastdds]: echoed=" << lis.echoed.load() << "\n";
    dp->delete_contained_entities();
    DomainParticipantFactory::get_instance()->delete_participant(dp);
    return 0;
}

template <typename T, typename PubSub>
class VerifyListener : public DataReaderListener {
public:
    void on_data_available(DataReader* r) override {
        T m; SampleInfo info;
        while (r->take_next_sample(&m, &info) == RETCODE_OK)
            if (info.valid_data) { std::lock_guard<std::mutex> lk(mu); last = m; got = true; cv.notify_one(); }
    }
    std::mutex mu; std::condition_variable cv; bool got{false}; T last;
};

template <typename T, typename PubSub>
int run_ping(uint64_t samples) {
    DomainParticipantQos pqos; apply_security(pqos);
    auto* dp = DomainParticipantFactory::get_instance()->create_participant(resolve_domain(), pqos);
    if (!dp) { std::cerr << "dp create failed\n"; return 1; }
    TypeSupport ts(new PubSub()); ts.register_type(dp);
    auto* tr = dp->create_topic(kReq, ts.get_type_name(), TOPIC_QOS_DEFAULT);
    auto* te = dp->create_topic(kEcho, ts.get_type_name(), TOPIC_QOS_DEFAULT);
    auto* pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT);
    auto* sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    DataWriterQos wq; qos_xcdr2(wq); auto* w = pub->create_datawriter(tr, wq);
    DataReaderQos rq; qos_xcdr2(rq);
    VerifyListener<T, PubSub> lis;
    auto* rd = sub->create_datareader(te, rq, &lis);
    if (!w || !rd) { std::cerr << "ping ep create failed\n"; return 1; }
    // wait for match
    for (int i = 0; i < 80; ++i) {
        PublicationMatchedStatus pm; SubscriptionMatchedStatus sm;
        w->get_publication_matched_status(pm); rd->get_subscription_matched_status(sm);
        if (pm.current_count > 0 && sm.current_count > 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    uint64_t ok = 0, miss = 0, bad = 0;
    for (uint32_t seq = 0; seq < samples; ++seq) {
        T msg = build((T*)nullptr, seq);
        bool done = false;
        for (int attempt = 0; attempt < 5 && !done; ++attempt) {
            { std::lock_guard<std::mutex> lk(lis.mu); lis.got = false; }
            w->write(&msg);
            std::unique_lock<std::mutex> lk(lis.mu);
            if (lis.cv.wait_for(lk, std::chrono::milliseconds(300), [&] { return lis.got; })) {
                if (seq_of(lis.last) == seq && content_eq(lis.last, msg)) { ++ok; done = true; }
                else { ++bad; done = true; }  // content corruption -- never retry
            }
        }
        if (!done) ++miss;
    }
    std::cout << "ping[fastdds]: type=" << ts.get_type_name() << " ok=" << ok << " miss=" << miss << " bad=" << bad << "\n";
    if (ok > 0 && bad == 0 && ok >= samples / 2) std::cout << "PROOF_OK type=" << ts.get_type_name() << " ok=" << ok << "/" << samples << "\n";
    else std::cout << "PROOF_FAIL type=" << ts.get_type_name() << " ok=" << ok << " miss=" << miss << " bad=" << bad << "\n";
    dp->delete_contained_entities();
    DomainParticipantFactory::get_instance()->delete_participant(dp);
    return (bad == 0 && ok > 0) ? 0 : 1;
}

template <typename T, typename PubSub>
int dispatch(const std::string& mode, uint64_t arg) {
    if (mode == "pong") return run_pong<T, PubSub>(arg ? arg : 30);
    if (mode == "ping") return run_ping<T, PubSub>(arg ? arg : 200);
    std::cerr << "unknown mode\n"; return 2;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "usage: fastdds_secxt ping|pong --type T [--samples N|--secs N]\n"; return 2; }
    std::string mode = argv[1], type = "mutable"; uint64_t arg = 0;
    for (int i = 2; i + 1 < argc; i += 2) {
        std::string f = argv[i];
        if (f == "--type") type = argv[i+1];
        else if (f == "--samples" || f == "--secs") arg = std::stoull(argv[i+1]);
    }
    if (type == "mutable")    return dispatch<secxt::MutableMsg,    secxt::MutableMsgPubSubType>(mode, arg);
    if (type == "appendable") return dispatch<secxt::AppendableMsg, secxt::AppendableMsgPubSubType>(mode, arg);
    if (type == "keyed")      return dispatch<secxt::KeyedMsg,      secxt::KeyedMsgPubSubType>(mode, arg);
    if (type == "nested")     return dispatch<secxt::NestedMsg,     secxt::NestedMsgPubSubType>(mode, arg);
    std::cerr << "unknown --type " << type << "\n"; return 2;
}
