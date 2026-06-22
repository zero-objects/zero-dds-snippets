// SPDX-License-Identifier: Apache-2.0
//
// Cyclone DDS secure XTypes interop peer. Security is configured entirely via
// CYCLONEDDS_URI (the per-profile cyclone_<role>.xml from gen_profile.sh) -- no
// security code here. idlc -l cxx generates the typed XCDR2 mutable/appendable
// framing. This app echoes (pong) or originates+verifies (ping) one
// XTypes-heavy topic.
//
// Build: g++ -std=c++17 cyclone_secxt.cpp secure_xtypes.cpp -lddscxx -lddsc -lpthread

#include "dds/dds.hpp"
#include "secure_xtypes.hpp"

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

using namespace org::eclipse::cyclonedds;

namespace {

uint32_t resolve_domain() {
    if (const char* s = std::getenv("ZERODDS_BENCH_DOMAIN")) try { return (uint32_t)std::stoul(s); } catch (...) {}
    return 200;
}
const char* kReq = "SecXT_Request";
const char* kEcho = "SecXT_Echo";

// Deterministic builders/comparators -- identical content rules to the other peers.
secxt::MutableMsg build(secxt::MutableMsg*, uint32_t seq) {
    secxt::MutableMsg m; m.seq(seq); m.a((int32_t)seq*7-3); m.b(3.14159*seq);
    m.label("mut-"+std::to_string(seq)); m.payload(std::vector<uint8_t>(64,(uint8_t)(seq&0xFF))); return m; }
secxt::AppendableMsg build(secxt::AppendableMsg*, uint32_t seq) {
    secxt::AppendableMsg m; m.seq(seq); m.t_send_ns(seq); m.label("app-"+std::to_string(seq));
    m.payload(std::vector<uint8_t>(64,(uint8_t)(seq&0xFF))); return m; }
secxt::KeyedMsg build(secxt::KeyedMsg*, uint32_t seq) {
    secxt::KeyedMsg m; m.region((int32_t)(seq%4)); m.unit((int32_t)seq); m.seq(seq);
    m.payload(std::vector<uint8_t>(64,(uint8_t)(seq&0xFF))); return m; }
secxt::NestedMsg build(secxt::NestedMsg*, uint32_t seq) {
    secxt::NestedMsg m; m.seq(seq);
    secxt::Inner h; h.x((int32_t)seq); h.y(2.71*seq); h.tag("head-"+std::to_string(seq)); m.head(h);
    std::vector<secxt::Inner> items;
    for (int i=0;i<3;++i){ secxt::Inner it; it.x((int32_t)(seq*10+i)); it.y((double)i); it.tag("it"+std::to_string(i)); items.push_back(it);}
    m.items(items); m.payload(std::vector<uint8_t>(64,(uint8_t)(seq&0xFF))); return m; }

uint32_t seq_of(const secxt::MutableMsg& m) { return m.seq(); }
uint32_t seq_of(const secxt::AppendableMsg& m) { return m.seq(); }
uint32_t seq_of(const secxt::KeyedMsg& m) { return m.seq(); }
uint32_t seq_of(const secxt::NestedMsg& m) { return m.seq(); }

bool content_eq(const secxt::MutableMsg& a, const secxt::MutableMsg& b) {
    return a.seq()==b.seq()&&a.a()==b.a()&&a.label()==b.label()&&a.payload()==b.payload(); }
bool content_eq(const secxt::AppendableMsg& a, const secxt::AppendableMsg& b) {
    return a.seq()==b.seq()&&a.t_send_ns()==b.t_send_ns()&&a.label()==b.label()&&a.payload()==b.payload(); }
bool content_eq(const secxt::KeyedMsg& a, const secxt::KeyedMsg& b) {
    return a.seq()==b.seq()&&a.region()==b.region()&&a.unit()==b.unit()&&a.payload()==b.payload(); }
bool content_eq(const secxt::NestedMsg& a, const secxt::NestedMsg& b) {
    if(!(a.seq()==b.seq()&&a.head().x()==b.head().x()&&a.head().tag()==b.head().tag()&&a.items().size()==b.items().size())) return false;
    for(size_t i=0;i<a.items().size();++i) if(!(a.items()[i].x()==b.items()[i].x()&&a.items()[i].tag()==b.items()[i].tag())) return false;
    return a.payload()==b.payload(); }

template <typename T>
class PongListener : public dds::sub::NoOpDataReaderListener<T> {
public:
    explicit PongListener(dds::pub::DataWriter<T>& w) : w_(w) {}
    void on_data_available(dds::sub::DataReader<T>& reader) override {
        for (auto& s : reader.take()) if (s.info().valid()) { w_.write(s.data()); ++echoed; }
    }
    std::atomic<uint64_t> echoed{0};
private:
    dds::pub::DataWriter<T>& w_;
};

dds::pub::qos::DataWriterQos wq(dds::pub::Publisher& p) {
    auto q = p.default_datawriter_qos();
    q << dds::core::policy::Reliability::Reliable() << dds::core::policy::History::KeepLast(64);
    q.policy(dds::core::policy::DataRepresentation({dds::core::policy::DataRepresentationId::XCDR2}));
    return q;
}
dds::sub::qos::DataReaderQos rq(dds::sub::Subscriber& s) {
    auto q = s.default_datareader_qos();
    q << dds::core::policy::Reliability::Reliable() << dds::core::policy::History::KeepLast(64);
    q.policy(dds::core::policy::DataRepresentation({dds::core::policy::DataRepresentationId::XCDR2}));
    return q;
}

template <typename T>
int run_pong(uint64_t secs) {
    dds::domain::DomainParticipant dp(resolve_domain());
    dds::topic::Topic<T> tr(dp, kReq), te(dp, kEcho);
    dds::pub::Publisher pub(dp); dds::sub::Subscriber sub(dp);
    dds::pub::DataWriter<T> w(pub, te, wq(pub));
    PongListener<T> lis(w);
    dds::sub::DataReader<T> rd(sub, tr, rq(sub), &lis, dds::core::status::StatusMask::data_available());
    std::cout << "pong[cyclone]: matched\n" << std::flush;
    std::this_thread::sleep_for(std::chrono::seconds(secs));
    std::cout << "pong[cyclone]: echoed=" << lis.echoed.load() << "\n";
    return 0;
}

template <typename T>
struct VState { std::mutex mu; std::condition_variable cv; bool got{false}; T last; };

template <typename T>
class PingListener : public dds::sub::NoOpDataReaderListener<T> {
public:
    explicit PingListener(VState<T>& st) : st_(st) {}
    void on_data_available(dds::sub::DataReader<T>& reader) override {
        for (auto& s : reader.take()) if (s.info().valid()) {
            std::lock_guard<std::mutex> lk(st_.mu); st_.last = s.data(); st_.got = true; st_.cv.notify_one();
        }
    }
private:
    VState<T>& st_;
};

template <typename T>
int run_ping(uint64_t samples) {
    dds::domain::DomainParticipant dp(resolve_domain());
    dds::topic::Topic<T> tr(dp, kReq), te(dp, kEcho);
    dds::pub::Publisher pub(dp); dds::sub::Subscriber sub(dp);
    dds::pub::DataWriter<T> w(pub, tr, wq(pub));
    VState<T> st;
    PingListener<T> lis(st);
    dds::sub::DataReader<T> rd(sub, te, rq(sub), &lis, dds::core::status::StatusMask::data_available());
    // wait for match
    for (int i = 0; i < 80; ++i) {
        if (w.publication_matched_status().current_count() > 0 &&
            rd.subscription_matched_status().current_count() > 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    uint64_t ok = 0, miss = 0, bad = 0;
    for (uint32_t seq = 0; seq < samples; ++seq) {
        T msg = build((T*)nullptr, seq);
        bool done = false;
        for (int attempt = 0; attempt < 5 && !done; ++attempt) {
            { std::lock_guard<std::mutex> lk(st.mu); st.got = false; }
            w.write(msg);
            std::unique_lock<std::mutex> lk(st.mu);
            if (st.cv.wait_for(lk, std::chrono::milliseconds(300), [&]{ return st.got; })) {
                if (seq_of(st.last) == seq && content_eq(st.last, msg)) { ++ok; done = true; }
                else { ++bad; done = true; }  // content corruption -- never retry
            }
        }
        if (!done) ++miss;
    }
    std::cout << "ping[cyclone]: ok=" << ok << " miss=" << miss << " bad=" << bad << "\n";
    if (ok > 0 && bad == 0 && ok >= samples / 2) std::cout << "PROOF_OK ok=" << ok << "/" << samples << "\n";
    else std::cout << "PROOF_FAIL ok=" << ok << " miss=" << miss << " bad=" << bad << "\n";
    return (bad == 0 && ok > 0) ? 0 : 1;
}

template <typename T>
int dispatch(const std::string& mode, uint64_t arg) {
    if (mode == "pong") return run_pong<T>(arg ? arg : 30);
    if (mode == "ping") return run_ping<T>(arg ? arg : 200);
    std::cerr << "unknown mode\n"; return 2;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "usage: cyclone_secxt ping|pong --type T [--samples N|--secs N]\n"; return 2; }
    std::string mode = argv[1], type = "mutable"; uint64_t arg = 0;
    for (int i = 2; i + 1 < argc; i += 2) {
        std::string f = argv[i];
        if (f == "--type") type = argv[i+1];
        else if (f == "--samples" || f == "--secs") arg = std::stoull(argv[i+1]);
    }
    try {
        if (type == "mutable")    return dispatch<secxt::MutableMsg>(mode, arg);
        if (type == "appendable") return dispatch<secxt::AppendableMsg>(mode, arg);
        if (type == "keyed")      return dispatch<secxt::KeyedMsg>(mode, arg);
        if (type == "nested")     return dispatch<secxt::NestedMsg>(mode, arg);
    } catch (const dds::core::Exception& e) {
        std::cerr << "cyclone exception: " << e.what() << "\n"; return 1;
    }
    std::cerr << "unknown --type " << type << "\n"; return 2;
}
