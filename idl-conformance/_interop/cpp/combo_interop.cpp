// idl-conformance / _interop / cpp — WIRE-level XCDR2 interop golden producer.
//
// Two modes:
//   ENCODE            -> write canonical combo::Telemetry as raw XCDR2-LE bytes
//                        to ../goldens/cpp.bin
//   DECODE <file>     -> read raw XCDR2 bytes, reconstruct combo::Telemetry,
//                        assert EVERY field == the canonical sample, exit 0 on
//                        match else nonzero (print a field-by-field diff).
//
// The generated header is header-only: topic_type_support<T>::encode/decode
// implement the XCDR2 codec entirely in headers, so no DDS runtime link is
// needed — the emitted bytes ARE the wire.

#include "gen/20_mixed_combo.hpp"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

using TS = dds::topic::topic_type_support<combo::Telemetry>;

// ---- The canonical sample, IDENTICAL across every language --------------
static combo::Telemetry canonical() {
    combo::Telemetry t;
    t.unitId(4242);                          // @key long
    t.region("eu-central-1");                // @key string<32>
    t.mode(combo::Mode::MODE_ACTIVE);        // enum, index 1
    t.batteryCurrent(13.75);                 // typedef double

    std::vector<combo::Sample> hist;         // sequence<Sample>
    hist.emplace_back(1, 0.5);
    hist.emplace_back(2, 1.5);
    hist.emplace_back(3, -2.25);
    t.history(hist);

    // union switch(Mode): MODE_ACTIVE -> activeRate (double) = 60.0
    t.reading()._d(combo::Mode::MODE_ACTIVE);
    t.reading().value() = 60.0;

    std::map<std::string, int32_t> ctr;      // map<string,long>, key-sorted
    ctr["drops"] = 3;
    ctr["packets"] = 100;
    t.counters(ctr);

    t.calibration(0.001);                    // @optional double, present

    std::array<int32_t, 4> win{{10, 20, 30, 40}};
    t.window(win);
    return t;
}

static int g_fail = 0;
template <typename A, typename B>
static void check(const char* what, const A& got, const B& want) {
    if (!(got == want)) {
        ++g_fail;
        std::cout << "  [FAIL] " << what << "  got != want\n";
    }
}

static int do_decode(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::cerr << "cannot open " << path << "\n";
        return 2;
    }
    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
    combo::Telemetry got =
        TS::decode(buf.data(), buf.size(), dds::topic::xcdr2::XcdrVersion::Xcdr2);
    combo::Telemetry want = canonical();

    check("unitId", got.unitId(), want.unitId());
    check("region", got.region(), want.region());
    check("mode", static_cast<int>(got.mode()), static_cast<int>(want.mode()));
    check("batteryCurrent", got.batteryCurrent(), want.batteryCurrent());

    check("history.size", got.history().size(), want.history().size());
    if (got.history().size() == want.history().size()) {
        for (size_t i = 0; i < got.history().size(); ++i) {
            check("history[].seq", got.history()[i].seq(), want.history()[i].seq());
            check("history[].value", got.history()[i].value(), want.history()[i].value());
        }
    }

    check("counters.size", got.counters().size(), want.counters().size());
    check("counters[drops]", got.counters().count("drops") ? got.counters().at("drops") : -1, 3);
    check("counters[packets]", got.counters().count("packets") ? got.counters().at("packets") : -1, 100);

    check("calibration.present", got.calibration().has_value(), want.calibration().has_value());
    if (got.calibration().has_value() && want.calibration().has_value())
        check("calibration.value", *got.calibration(), *want.calibration());

    check("reading._d", static_cast<int>(got.reading()._d()), static_cast<int>(want.reading()._d()));
    check("reading.activeRate", std::get<double>(got.reading().value()),
          std::get<double>(want.reading().value()));

    for (int i = 0; i < 4; ++i)
        check("window[]", got.window()[i], want.window()[i]);

    if (g_fail == 0) {
        std::cout << "DECODE OK: all fields match canonical.\n";
        return 0;
    }
    std::cout << "DECODE FAILED: " << g_fail << " mismatch(es).\n";
    return 1;
}

static int do_encode(const char* path) {
    std::vector<uint8_t> wire =
        TS::encode(canonical(), dds::topic::xcdr2::XcdrVersion::Xcdr2);
    std::ofstream f(path, std::ios::binary);
    if (!f) {
        std::cerr << "cannot write " << path << "\n";
        return 2;
    }
    f.write(reinterpret_cast<const char*>(wire.data()),
            static_cast<std::streamsize>(wire.size()));
    std::cout << "ENCODE OK: wrote " << wire.size() << " bytes to " << path << "\n";
    return 0;
}

int main(int argc, char** argv) {
    if (argc >= 2 && std::string(argv[1]) == "ENCODE") {
        const char* out = (argc >= 3) ? argv[2]
                                      : "../goldens/cpp.bin";
        return do_encode(out);
    }
    if (argc >= 3 && std::string(argv[1]) == "DECODE") {
        return do_decode(argv[2]);
    }
    std::cerr << "usage: " << argv[0] << " ENCODE [out.bin] | DECODE <file>\n";
    return 64;
}
