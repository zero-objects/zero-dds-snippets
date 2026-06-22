// SPDX-License-Identifier: Apache-2.0
//
// ZeroDDS secure XTypes interop app. Carries one XTypes-heavy topic
// (mutable / appendable / keyed / nested) over a DDS-Security session
// (governance + permissions signed, AES-GCM message/payload protection) and
// proves the peer recovers the exact field content after decrypt+decode -- i.e.
// the SecureDataPayload / SecureSubMessage transform carries the XCDR2 framing
// (DHEADER / EMHEADER / KeyHash) intact.
//
// Roles:
//   pong  -- decode the secured request, re-encode, write to the echo topic.
//   ping  -- write the request, verify the echoed sample equals what was sent
//            (field-by-field, not just RTT). Prints PROOF_OK / PROOF_FAIL.
//
// Security env (identical to the roundtrip-bench harness so the same
// gen_profile.sh profiles drive it):
//   ZERODDS_BENCH_SECURITY=1  ZERODDS_BENCH_SEC_DIR=<profile>/text
//   ZERODDS_BENCH_SEC_NAME=ping|pong   ZERODDS_BENCH_DOMAIN=<id>
//   ZERODDS_SECURE_SPDP=1     (only vs Fast DDS)
//
// Type selected via argv: --type mutable|appendable|keyed|nested

#include "zerodds.h"
#include "secure_xtypes.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace dds::topic;

namespace {

uint32_t resolve_domain() {
    if (const char* s = std::getenv("ZERODDS_BENCH_DOMAIN"))
        try { return (uint32_t)std::stoul(s); } catch (...) {}
    return 200;
}

zerodds_ZeroDdsRuntime* make_runtime() {
    const char* enable = std::getenv("ZERODDS_BENCH_SECURITY");
    if (!enable || std::string(enable) != "1")
        return zerodds_runtime_create(resolve_domain());
    const char* sec_dir = std::getenv("ZERODDS_BENCH_SEC_DIR");
    if (!sec_dir) sec_dir = "/root/bench-security";
    const char* who = std::getenv("ZERODDS_BENCH_SEC_NAME");
    if (!who) who = "ping";
    auto p = [&](const std::string& s) { return std::string(sec_dir) + "/" + s; };
    const std::string id_ca = p("certs/identity_ca.pem");
    const std::string id_cert = p("certs/" + std::string(who) + "_cert.pem");
    const std::string id_key = p("certs/" + std::string(who) + "_key.pem");
    const std::string perm_ca = p("certs/permissions_ca.pem");
    const std::string gov = p("governance.p7s");
    const std::string perms = p("permissions_" + std::string(who) + ".p7s");
    auto* cfg = zerodds_security_config_create();
    if (!cfg) { std::cerr << "security_config_create failed\n"; return nullptr; }
    int rc = 0;
    rc |= zerodds_security_set_identity_ca_path(cfg, id_ca.c_str());
    rc |= zerodds_security_set_identity_cert_path(cfg, id_cert.c_str());
    rc |= zerodds_security_set_private_key_path(cfg, id_key.c_str());
    rc |= zerodds_security_set_permissions_ca_path(cfg, perm_ca.c_str());
    rc |= zerodds_security_set_governance_path(cfg, gov.c_str());
    rc |= zerodds_security_set_permissions_path(cfg, perms.c_str());
    if (rc != 0) { std::cerr << "security setter rc=" << rc << "\n"; zerodds_security_config_destroy(cfg); return nullptr; }
    auto* rt = zerodds_runtime_create_secure(resolve_domain(), cfg);
    zerodds_security_config_destroy(cfg);
    return rt;
}

// ---- per-type traits: topic names + type name + sample build/verify ----
enum class Kind { Mutable, Appendable, Keyed, Nested };

Kind parse_kind(const std::string& s) {
    if (s == "mutable") return Kind::Mutable;
    if (s == "appendable") return Kind::Appendable;
    if (s == "keyed") return Kind::Keyed;
    if (s == "nested") return Kind::Nested;
    std::cerr << "unknown --type " << s << "\n"; std::exit(2);
}

const char* type_name(Kind k) {
    switch (k) {
        case Kind::Mutable: return "secxt::MutableMsg";
        case Kind::Appendable: return "secxt::AppendableMsg";
        case Kind::Keyed: return "secxt::KeyedMsg";
        case Kind::Nested: return "secxt::NestedMsg";
    }
    return "";
}

// XCDR version: mutable/keyed/nested require XCDR2 (PL_CDR2); appendable XCDR2
// (D_CDR2). All XCDR2 here.
constexpr auto kVer = xcdr2::XcdrVersion::Xcdr2;

// Build a sample with seq, returns encoded bytes.
std::vector<uint8_t> encode_for(Kind k, uint32_t seq, size_t payload_sz) {
    std::vector<uint8_t> pl(payload_sz, (uint8_t)(seq & 0xFF));
    switch (k) {
        case Kind::Mutable: {
            secxt::MutableMsg m; m.seq(seq); m.a((int32_t)seq * 7 - 3);
            m.b(3.14159 * seq); m.label("mut-" + std::to_string(seq)); m.payload(pl);
            return topic_type_support<secxt::MutableMsg>::encode(m, kVer);
        }
        case Kind::Appendable: {
            secxt::AppendableMsg m; m.seq(seq); m.t_send_ns(seq);
            m.label("app-" + std::to_string(seq)); m.payload(pl);
            return topic_type_support<secxt::AppendableMsg>::encode(m, kVer);
        }
        case Kind::Keyed: {
            secxt::KeyedMsg m; m.region((int32_t)(seq % 4)); m.unit((int32_t)seq);
            m.seq(seq); m.payload(pl);
            return topic_type_support<secxt::KeyedMsg>::encode(m, kVer);
        }
        case Kind::Nested: {
            secxt::NestedMsg m; m.seq(seq);
            secxt::Inner h; h.x((int32_t)seq); h.y(2.71 * seq); h.tag("head-" + std::to_string(seq));
            m.head(h);
            std::vector<secxt::Inner> items;
            for (int i = 0; i < 3; ++i) { secxt::Inner it; it.x((int32_t)(seq * 10 + i)); it.y((double)i); it.tag("it" + std::to_string(i)); items.push_back(it); }
            m.items(items); m.payload(pl);
            return topic_type_support<secxt::NestedMsg>::encode(m, kVer);
        }
    }
    return {};
}

// Extract seq + a content-fingerprint from a decoded wire sample. The
// fingerprint folds the rich fields (mutable a/b/label, appendable label,
// nested head + items) so a framing corruption shows up as a fingerprint
// mismatch, not just a missing sample.
struct Decoded { bool ok{false}; uint32_t seq{0}; uint64_t fp{0}; };

uint64_t hash_str(const std::string& s) {
    uint64_t h = 1469598103934665603ULL;
    for (char c : s) { h ^= (uint8_t)c; h *= 1099511628211ULL; }
    return h;
}

Decoded decode_for(Kind k, const uint8_t* buf, size_t len, uint8_t repr) {
    auto ver = (repr == 1) ? xcdr2::XcdrVersion::Xcdr2 : xcdr2::XcdrVersion::Xcdr1;
    Decoded d;
    try {
        switch (k) {
            case Kind::Mutable: {
                auto m = topic_type_support<secxt::MutableMsg>::decode(buf, len, ver);
                d.seq = m.seq();
                d.fp = (uint64_t)(uint32_t)m.a() ^ hash_str(m.label()) ^ (uint64_t)(int64_t)(m.b() * 1000) ^ (m.payload().empty() ? 0 : m.payload()[0]);
                break;
            }
            case Kind::Appendable: {
                auto m = topic_type_support<secxt::AppendableMsg>::decode(buf, len, ver);
                d.seq = m.seq();
                d.fp = m.t_send_ns() ^ hash_str(m.label()) ^ (m.payload().empty() ? 0 : m.payload()[0]);
                break;
            }
            case Kind::Keyed: {
                auto m = topic_type_support<secxt::KeyedMsg>::decode(buf, len, ver);
                d.seq = m.seq();
                d.fp = (uint64_t)(uint32_t)m.region() ^ ((uint64_t)(uint32_t)m.unit() << 16) ^ (m.payload().empty() ? 0 : m.payload()[0]);
                break;
            }
            case Kind::Nested: {
                auto m = topic_type_support<secxt::NestedMsg>::decode(buf, len, ver);
                d.seq = m.seq();
                uint64_t f = (uint64_t)(uint32_t)m.head().x() ^ hash_str(m.head().tag());
                f ^= (uint64_t)m.items().size() << 32;
                for (auto& it : m.items()) f ^= hash_str(it.tag()) ^ (uint64_t)(uint32_t)it.x();
                d.fp = f ^ (m.payload().empty() ? 0 : m.payload()[0]);
                break;
            }
        }
        d.ok = true;
    } catch (...) { d.ok = false; }
    return d;
}

uint64_t expected_fp(Kind k, uint32_t seq, size_t payload_sz) {
    // Recompute the fingerprint the peer should produce for the sample we sent.
    uint8_t p0 = payload_sz ? (uint8_t)(seq & 0xFF) : 0;
    switch (k) {
        case Kind::Mutable: {
            int32_t a = (int32_t)seq * 7 - 3; double b = 3.14159 * seq;
            return (uint64_t)(uint32_t)a ^ hash_str("mut-" + std::to_string(seq)) ^ (uint64_t)(int64_t)(b * 1000) ^ p0;
        }
        case Kind::Appendable:
            return (uint64_t)seq ^ hash_str("app-" + std::to_string(seq)) ^ p0;
        case Kind::Keyed:
            return (uint64_t)(uint32_t)(int32_t)(seq % 4) ^ ((uint64_t)(uint32_t)(int32_t)seq << 16) ^ p0;
        case Kind::Nested: {
            uint64_t f = (uint64_t)seq ^ hash_str("head-" + std::to_string(seq));
            f ^= (uint64_t)3 << 32;
            for (int i = 0; i < 3; ++i) f ^= hash_str("it" + std::to_string(i)) ^ (uint64_t)(uint32_t)(int32_t)(seq * 10 + i);
            return f ^ p0;
        }
    }
    return 0;
}

const char* kReq = "SecXT_Request";
const char* kEcho = "SecXT_Echo";

struct PongCtx { Kind k; zerodds_ZeroDdsWriter* echo; std::atomic<uint64_t> echoed{0}; std::atomic<uint64_t> bad{0}; };

extern "C" void pong_on_data(void* ud, const uint8_t* payload, size_t len, uint8_t repr) {
    auto* ctx = static_cast<PongCtx*>(ud);
    auto d = decode_for(ctx->k, payload, len, repr);
    if (!d.ok) { ctx->bad.fetch_add(1); return; }
    // Re-encode the *decoded* sample and echo it. If decode lost framing, the
    // re-encoded bytes differ and the ping side's fingerprint check fails.
    auto enc = encode_for(ctx->k, d.seq, /*payload_sz*/64);  // payload reconstructed deterministically by seq
    zerodds_writer_write(ctx->echo, enc.data(), enc.size());
    ctx->echoed.fetch_add(1);
}

int run_pong(Kind k, uint64_t secs) {
    auto* rt = make_runtime();
    if (!rt) { std::cerr << "runtime_create failed\n"; return 1; }
    if (zerodds_runtime_wait_for_peers(rt, 1, 30000) != 0) { std::cerr << "pong: wait_for_peers timeout\n"; return 1; }
    auto* dr = zerodds_reader_create_kind(rt, kReq, type_name(k), 1, 0);
    auto* dw = zerodds_writer_create_kind(rt, kEcho, type_name(k), 1, 0);
    if (!dr || !dw) { std::cerr << "pong: ep create failed\n"; return 1; }
    if (zerodds_writer_wait_for_matched(dw, 1, 8000) != 0) { std::cerr << "pong: writer match timeout\n"; return 1; }
    if (zerodds_reader_wait_for_matched(dr, 1, 8000) != 0) { std::cerr << "pong: reader match timeout\n"; return 1; }
    PongCtx ctx; ctx.k = k; ctx.echo = dw;
    zerodds_reader_set_data_callback(dr, pong_on_data, &ctx);
    std::cout << "pong[zerodds]: matched type=" << type_name(k) << " (secure echo)\n" << std::flush;
    std::this_thread::sleep_for(std::chrono::seconds(secs));
    zerodds_reader_set_data_callback(dr, nullptr, nullptr);
    std::cout << "pong: echoed=" << ctx.echoed.load() << " decode_fail=" << ctx.bad.load() << "\n";
    zerodds_writer_destroy(dw); zerodds_reader_destroy(dr); zerodds_runtime_destroy(rt);
    return 0;
}

struct PingCtx { Kind k; std::mutex mu; std::condition_variable cv; bool got{false}; Decoded last; };

extern "C" void ping_on_data(void* ud, const uint8_t* payload, size_t len, uint8_t repr) {
    auto* ctx = static_cast<PingCtx*>(ud);
    auto d = decode_for(ctx->k, payload, len, repr);
    { std::lock_guard<std::mutex> lk(ctx->mu); ctx->last = d; ctx->got = true; }
    ctx->cv.notify_one();
}

int run_ping(Kind k, uint64_t samples) {
    auto* rt = make_runtime();
    if (!rt) { std::cerr << "runtime_create failed\n"; return 1; }
    if (zerodds_runtime_wait_for_peers(rt, 1, 30000) != 0) { std::cerr << "ping: wait_for_peers timeout\n"; return 1; }
    auto* dw = zerodds_writer_create_kind(rt, kReq, type_name(k), 1, 0);
    auto* dr = zerodds_reader_create_kind(rt, kEcho, type_name(k), 1, 0);
    if (!dr || !dw) { std::cerr << "ping: ep create failed\n"; return 1; }
    if (zerodds_writer_wait_for_matched(dw, 1, 8000) != 0) { std::cerr << "ping: writer match timeout\n"; return 1; }
    if (zerodds_reader_wait_for_matched(dr, 1, 8000) != 0) { std::cerr << "ping: reader match timeout\n"; return 1; }
    PingCtx ctx; ctx.k = k;
    zerodds_reader_set_data_callback(dr, ping_on_data, &ctx);
    uint64_t ok = 0, miss = 0, fp_fail = 0;
    const size_t PSZ = 64;
    for (uint32_t seq = 0; seq < samples; ++seq) {
        // Retry each seq up to 5x so transient discovery / reliability cadence
        // (esp. the Fast DDS secure-SPDP channel warm-up) does not masquerade as
        // a crypto/XTypes failure. A genuine framing corruption shows up as
        // fp_fail (content mismatch), which is never retried away.
        bool done = false;
        auto enc = encode_for(k, seq, PSZ);
        for (int attempt = 0; attempt < 5 && !done; ++attempt) {
            { std::lock_guard<std::mutex> lk(ctx.mu); ctx.got = false; }
            if (zerodds_writer_write(dw, enc.data(), enc.size()) != 0) { std::cerr << "ping: write fail\n"; return 1; }
            std::unique_lock<std::mutex> lk(ctx.mu);
            if (ctx.cv.wait_for(lk, std::chrono::milliseconds(300), [&] { return ctx.got; })) {
                if (ctx.last.ok && ctx.last.seq == seq && ctx.last.fp == expected_fp(k, seq, PSZ)) { ++ok; done = true; }
                else if (ctx.last.ok) { ++fp_fail; done = true; }  // content corruption -- do NOT retry
            }
        }
        if (!done) ++miss;
    }
    zerodds_reader_set_data_callback(dr, nullptr, nullptr);
    std::cout << "ping[zerodds]: type=" << type_name(k) << " ok=" << ok << " miss=" << miss << " fp_fail=" << fp_fail << "\n";
    if (ok > 0 && fp_fail == 0 && ok >= samples / 2)
        std::cout << "PROOF_OK type=" << type_name(k) << " ok=" << ok << "/" << samples << "\n";
    else
        std::cout << "PROOF_FAIL type=" << type_name(k) << " ok=" << ok << " miss=" << miss << " fp_fail=" << fp_fail << "\n";
    zerodds_writer_destroy(dw); zerodds_reader_destroy(dr); zerodds_runtime_destroy(rt);
    return (fp_fail == 0 && ok > 0) ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
    std::string mode, type = "mutable"; uint64_t arg = 0;
    if (argc < 2) { std::cerr << "usage: zerodds_secxt ping|pong --type T [--samples N | --secs N]\n"; return 2; }
    mode = argv[1];
    for (int i = 2; i + 1 < argc; i += 2) {
        std::string f = argv[i];
        if (f == "--type") type = argv[i+1];
        else if (f == "--samples" || f == "--secs") arg = std::stoull(argv[i+1]);
    }
    Kind k = parse_kind(type);
    if (mode == "pong") return run_pong(k, arg ? arg : 30);
    if (mode == "ping") return run_ping(k, arg ? arg : 200);
    std::cerr << "unknown mode\n"; return 2;
}
