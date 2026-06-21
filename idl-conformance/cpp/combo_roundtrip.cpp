// idl-conformance / cpp — real-DCPS roundtrip of a multi-feature IDL combo type.
//
// Generated from tools/idlc/tests/conformance/fixtures/20_mixed_combo.idl with
//   zerodds-idlc generate 20_mixed_combo.idl --cpp -o gen
//
// combo::Telemetry exercises, in ONE keyed topic type:
//   enum (Mode), nested struct (Sample), sequence-of-struct (history),
//   bounded string<32> (region, @key), map<string,long> (counters),
//   @optional double (calibration), array long[4] (window), typedef
//   (CurrentInAmpsType=double), and @key (unitId + region).
//
// It publishes ONE fully-populated sample over a real ZeroDDS DCPS participant
// (domain 0, loopback) with TypedWriter<Telemetry>, reads it back with
// TypedReader<Telemetry>, and asserts every recovered field equals what was
// published — proving the data survived the XCDR2 wire, not just an in-memory
// encode/decode.
//
// KNOWN LIMITATIONS (see README):
//   1. The union member `reading` is silently dropped by the current cpp codegen
//      (encode/decode emit
//      "// xcdr2: member 'reading' not supported (nested/enum/map/fixed; skip)").
//      It is symmetric (skipped on both sides), so the rest of the sample still
//      round-trips, but the union value itself does NOT survive the wire and is
//      therefore excluded from the equality assertions.
//   2. The byte-oriented C-API write path does not carry the XCDR data-
//      representation tag, so `zerodds_reader_take` reports repr=0 (XCDR1) even
//      for an XCDR2 payload. We therefore use the byte-oriented Writer/Reader
//      and decode explicitly with the same XCDR2 version we encoded with —
//      rather than `TypedReader<T>`, which would trust the (wrong) repr tag and
//      mis-align the decode. The wire bytes themselves are byte-identical.

#include "gen/20_mixed_combo.hpp"
#include "zerodds/dds.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

static int g_failures = 0;

template <typename A, typename B>
static void check(const char* what, const A& got, const B& want) {
    bool ok = (got == want);
    std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << what << "\n";
    if (!ok) ++g_failures;
}

int main() {
    // --- Build one fully-populated sample -------------------------------
    combo::Telemetry sent;
    sent.unitId(4242);                              // @key long
    sent.region("eu-central-1");                    // @key bounded string<32>
    sent.mode(combo::Mode::MODE_ACTIVE);            // enum
    sent.batteryCurrent(13.75);                     // typedef double (CurrentInAmpsType)

    std::vector<combo::Sample> hist;                // sequence<Sample>
    hist.emplace_back(1, 100.5);
    hist.emplace_back(2, 200.25);
    hist.emplace_back(3, -3.5);
    sent.history(hist);

    // union member `reading` — populated, but does NOT round-trip (see README).
    sent.reading().value() = 0.987;
    sent.reading()._d(combo::Mode::MODE_ACTIVE);

    std::map<std::string, int32_t> ctr;             // map<string, long>
    ctr["ok"] = 7;
    ctr["err"] = 2;
    ctr["retry"] = 13;
    sent.counters(ctr);

    sent.calibration(0.0025);                       // @optional double (present)

    std::array<int32_t, 4> win{{10, 20, 30, 40}};   // long[4]
    sent.window(win);

    // --- Real DCPS participant + pub/sub loopback -----------------------
    // Byte-oriented Writer/Reader so we control the XCDR version explicitly
    // (see Known limitation 2). Topic type name + keyed flag come straight
    // from the generated trait, so it is still the real keyed combo topic.
    using TS = dds::topic::topic_type_support<combo::Telemetry>;
    zerodds::Runtime rt(0);
    zerodds::Writer w(rt, "ComboTelemetry", TS::type_name(), /*reliable=*/true);
    zerodds::Reader r(rt, "ComboTelemetry", TS::type_name(), /*reliable=*/true);

    if (!w.wait_for_matched(1, 5000)) {
        std::cerr << "writer never matched a reader\n";
        return 2;
    }
    if (!r.wait_for_matched(1, 5000)) {
        std::cerr << "reader never matched a writer\n";
        return 2;
    }

    // Encode XCDR2 and publish the raw bytes over DCPS.
    std::vector<uint8_t> wire = TS::encode(sent, dds::topic::xcdr2::XcdrVersion::Xcdr2);
    w.write(wire);

    // Poll for the sample (event-friendly short waits, tight window), then
    // decode with the SAME XCDR2 version we encoded with.
    combo::Telemetry got;
    bool received = false;
    for (int i = 0; i < 50 && !received; ++i) {
        std::vector<uint8_t> payload = r.take();
        if (!payload.empty()) {
            got = TS::decode(payload.data(), payload.size(),
                             dds::topic::xcdr2::XcdrVersion::Xcdr2);
            received = true;
            break;
        }
        std::this_thread::sleep_for(20ms);
    }
    if (!received) {
        std::cerr << "no sample received over DCPS loopback\n";
        return 3;
    }

    std::cout << "Recovered sample over DCPS loopback — verifying fields:\n";

    // --- Assert every wire-supported field survived ---------------------
    check("@key long unitId",          got.unitId(),          sent.unitId());
    check("@key string<32> region",    got.region(),          sent.region());
    check("enum mode",                 static_cast<int>(got.mode()),
                                       static_cast<int>(sent.mode()));
    check("typedef double battery",    got.batteryCurrent(),  sent.batteryCurrent());

    check("sequence<Sample> size",     got.history().size(),  sent.history().size());
    if (got.history().size() == sent.history().size()) {
        for (size_t i = 0; i < got.history().size(); ++i) {
            check("  history[i].seq",   got.history()[i].seq(),   sent.history()[i].seq());
            check("  history[i].value", got.history()[i].value(), sent.history()[i].value());
        }
    }

    check("map<string,long> size",     got.counters().size(), sent.counters().size());
    check("  counters[ok]",            got.counters().at("ok"),    7);
    check("  counters[err]",           got.counters().at("err"),   2);
    check("  counters[retry]",         got.counters().at("retry"), 13);

    check("@optional has_value",       got.calibration().has_value(),
                                       sent.calibration().has_value());
    if (got.calibration().has_value() && sent.calibration().has_value()) {
        check("@optional value",       *got.calibration(),    *sent.calibration());
    }

    check("array long[4] [0]",         got.window()[0],       sent.window()[0]);
    check("array long[4] [1]",         got.window()[1],       sent.window()[1]);
    check("array long[4] [2]",         got.window()[2],       sent.window()[2]);
    check("array long[4] [3]",         got.window()[3],       sent.window()[3]);

    std::cout << "\nNote: union member 'reading' is excluded — the cpp codegen "
                 "drops it on the wire (see README Known limitations).\n";

    if (g_failures == 0) {
        std::cout << "\nROUNDTRIP OK: all wire-supported combo features survived DCPS.\n";
        return 0;
    }
    std::cout << "\nROUNDTRIP FAILED: " << g_failures << " field(s) did not match.\n";
    return 1;
}
