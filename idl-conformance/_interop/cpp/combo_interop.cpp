// idl-conformance / _interop / cpp — the combo::Telemetry column.
//
//   combo_interop ENCODE <out.bin>   write the canonical sample as raw XCDR2-LE
//   combo_interop DECODE <in.bin>    decode, assert every field == canonical
//
// gen_combo/combo.hpp is `zerodds-idlc generate --lang c++` output of
// ../combo.idl over runtime/cpp; `dds::topic::topic_type_support<T>` is its
// codec. Values are CANONICAL.md.
#include "gen_combo/combo.hpp"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using TS = dds::topic::topic_type_support<combo::Telemetry>;

static combo::Telemetry canonical() {
    combo::Telemetry t{};
    t.unitId = 4242;                          // @key long
    t.region = "eu-central-1";                // @key string<32>
    t.mode = combo::Mode::MODE_ACTIVE;        // enum, index 1
    t.batteryCurrent = 13.75;                 // typedef double
    t.history = {{1, 0.5}, {2, 1.5}, {3, -2.25}};
    t.reading._d = combo::Mode::MODE_ACTIVE;  // union switch(Mode) -> activeRate
    t.reading.activeRate = 60.0;
    t.counters = {{"drops", 3}, {"packets", 100}};  // map<string,long>, key-sorted
    t.calibration = 0.001;                    // @optional double, present
    t.window = {10, 20, 30, 40};              // long[4]
    return t;
}

static int g_fail = 0;
#define CHK(cond, what) do { if (!(cond)) { ++g_fail; std::fprintf(stderr, "  [FAIL] %s\n", what); } } while (0)

static void check(const combo::Telemetry& g) {
    const combo::Telemetry w = canonical();
    CHK(g.unitId == w.unitId, "unitId");
    CHK(g.region == w.region, "region");
    CHK(g.mode == w.mode, "mode");
    CHK(g.batteryCurrent == w.batteryCurrent, "batteryCurrent");
    CHK(g.history.size() == w.history.size(), "history.size");
    if (g.history.size() == w.history.size()) {
        for (std::size_t i = 0; i < w.history.size(); ++i) {
            CHK(g.history[i].seq == w.history[i].seq, "history[].seq");
            CHK(g.history[i].value == w.history[i].value, "history[].value");
        }
    }
    CHK(g.reading._d == w.reading._d, "reading._d");
    CHK(g.reading.activeRate == w.reading.activeRate, "reading.activeRate");
    CHK(g.counters == w.counters, "counters");
    CHK(g.calibration.has_value() && *g.calibration == *w.calibration, "calibration");
    CHK(g.window == w.window, "window");
}

static int do_encode(const char* path) {
    const std::vector<std::uint8_t> wire = TS::encode(canonical());
    std::ofstream f(path, std::ios::binary);
    if (!f) { std::fprintf(stderr, "cannot write %s\n", path); return 2; }
    f.write(reinterpret_cast<const char*>(wire.data()), static_cast<std::streamsize>(wire.size()));
    if (!f) { std::fprintf(stderr, "incomplete write to %s\n", path); return 2; }
    std::printf("ENCODE OK: %zu bytes -> %s\n", wire.size(), path);
    return 0;
}

static int do_decode(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { std::fprintf(stderr, "cannot open %s\n", path); return 2; }
    const std::vector<std::uint8_t> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    try {
        check(TS::decode(buf.data(), buf.size(), zerodds::wire::Encoding::PlainCdr2, false));
    } catch (const std::exception& e) {
        ++g_fail;
        std::fprintf(stderr, "  [FAIL] decode: %s\n", e.what());
    }
    if (g_fail == 0) { std::printf("DECODE OK: every field equals canonical\n"); return 0; }
    std::printf("DECODE FAILED: %d mismatch(es)\n", g_fail);
    return 1;
}

int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "ENCODE") return do_encode(argv[2]);
    if (argc == 3 && std::string(argv[1]) == "DECODE") return do_decode(argv[2]);
    std::fprintf(stderr, "usage: %s ENCODE <out.bin> | DECODE <in.bin>\n", argv[0]);
    return 2;
}
