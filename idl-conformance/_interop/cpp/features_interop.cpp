// idl-conformance / _interop / cpp — per-feature XCDR2-LE golden producer.
//
//   <feature> ENCODE <out.bin>   -> write canonical feat::<T> as raw XCDR2-LE
//   <feature> DECODE <in.bin>    -> decode, assert == canonical, exit 0/1
//   -         BE                 -> decode the big-endian goldens of
//                                   ../../proofs/endianness, assert == canonical
//
//   -         XCDR1              -> built with -DFEATURES_XCDR1: decode the
//                                   XCDR1 goldens of ../../proofs/endianness,
//                                   little- and big-endian
//
// gen_feat/features.hpp is `zerodds-idlc generate --lang c++` output over
// runtime/cpp, gen_x1/features.hpp the same with `--encoding xcdr1`;
// `dds::topic::topic_type_support<T>` is the codec. Values are CANONICAL.md.
#ifdef FEATURES_XCDR1
#include "gen_x1/features.hpp"
static constexpr auto kEncoding = zerodds::wire::Encoding::PlainCdr1;
#else
#include "gen_feat/features.hpp"
static constexpr auto kEncoding = zerodds::wire::Encoding::PlainCdr2;
#endif

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

template <class T> using TS = dds::topic::topic_type_support<T>;

static int g_fail = 0;
#define CHK(cond, what) do { if (!(cond)) { ++g_fail; std::fprintf(stderr, "  [FAIL] %s\n", what); } } while (0)

// ---- canonical samples (CANONICAL.md) ---------------------------------------
static feat::WStr canon_wstr() { return {"caf\xc3\xa9", "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xf0\x9f\x8e\x89"}; }
static feat::Mut canon_mut() { return {1000000, 2.5, "ok"}; }
static feat::MutNest canon_mutnest() { return {9, {100, 1.25}, {{1, 0.5}, {2, 0.25}}}; }
static feat::OuterKey canon_outerkey() { return {{0x01020304, 0x05060708}, 999}; }
static feat::Bits canon_bits() { return {feat::READ | feat::EXEC, {5, 20}}; }
static feat::Tree canon_tree() { return {1, {{2, {{4, {}}}}, {3, {}}}}; }
static feat::Arr canon_arr() { return {{{{10, 11, 12}, {13, 14, 15}}}, {{{1, 2}, {3, 4}}}}; }
static feat::Prim canon_prim() {
    feat::Prim v{};
    v.i8 = -128; v.u8 = 255; v.i16 = -32768; v.u16 = 65535;
    v.i32 = INT32_MIN; v.u32 = UINT32_MAX; v.i64 = INT64_MIN; v.u64 = UINT64_MAX;
    v.f32 = 3.5f; v.f64 = -1234.5; v.b = true; v.o = 0xAB; v.ch = 'Z';
    return v;
}
// @bit_bound enum + map<long,Pt> + sequence<Sel>: h=BLUE; m={3:{11,12}}; sels=[Sel{_d=2,n=9}].
static feat::MapEnum canon_mapenum() {
    feat::Sel sel{};
    sel.n(9);  // the setter selects its case: _d() == 2
    feat::MapEnum v{};
    v.h = feat::Hue::H_BLUE;
    v.m.set_max_size(1);  // a map's capacity starts at 0; insert stays within it
    v.m.insert(3, feat::Pt{11, 12});
    v.sels = {sel};
    return v;
}
// map<long,long> primitive-valued, no collection DHEADER (XCDR2 §7.4.3.5): m={7:42,8:99}.
static feat::MapPrim canon_mapprim() {
    feat::MapPrim v{};
    v.m.set_max_size(2);
    v.m.insert(7, 42);
    v.m.insert(8, 99);
    return v;
}
// fixed<P,S> as its decimal text: price=123.45 (wire 12 34 5c), qty=1234 (01 23 4c).
static feat::Money canon_fixed() { return {"123.45", "1234"}; }

// ---- checks -------------------------------------------------------------------
static void check(const feat::WStr& g) {
    auto w = canon_wstr();
    CHK(g.label == w.label, "wstr.label"); CHK(g.text == w.text, "wstr.text");
}
static void check(const feat::Mut& g) { CHK(g.a == 1000000 && g.b == 2.5 && g.c == "ok", "mut"); }
static void check(const feat::MutNest& g) {
    CHK(g.tag == 9 && g.leaf.u == 100 && g.leaf.v == 1.25, "mutnest.tag/leaf");
    CHK(g.list.size() == 2 && g.list[0].u == 1 && g.list[0].v == 0.5 && g.list[1].u == 2 && g.list[1].v == 0.25,
        "mutnest.list");
}
static void check(const feat::OuterKey& g) { CHK(g.k.hi == 0x01020304 && g.k.lo == 0x05060708 && g.payload == 999, "outerkey"); }
static void check(const feat::Bits& g) {
    CHK(g.perm == (feat::READ | feat::EXEC), "bits.perm"); CHK(g.flags.kind == 5 && g.flags.prio == 20, "bits.flags");
}
static void check(const feat::Tree& g) {
    CHK(g.value == 1 && g.kids.size() == 2, "tree.root");
    if (g.kids.size() == 2) {
        CHK(g.kids[0].value == 2 && g.kids[0].kids.size() == 1 && g.kids[0].kids[0].value == 4, "tree.k0");
        CHK(g.kids[1].value == 3 && g.kids[1].kids.empty(), "tree.k1");
    }
}
static void check(const feat::Arr& g) {
    auto w = canon_arr();
    CHK(g.grid == w.grid, "arr.grid");
    CHK(g.shape[0].x == 1 && g.shape[0].y == 2 && g.shape[1].x == 3 && g.shape[1].y == 4, "arr.shape");
}
static void check(const feat::Prim& g) {
    auto w = canon_prim();
    CHK(g.i8 == w.i8 && g.u8 == w.u8 && g.i16 == w.i16 && g.u16 == w.u16, "prim.8/16");
    CHK(g.i32 == w.i32 && g.u32 == w.u32 && g.i64 == w.i64 && g.u64 == w.u64, "prim.32/64");
    CHK(g.f32 == w.f32 && g.f64 == w.f64 && g.b == w.b && g.o == w.o && g.ch == w.ch, "prim.rest");
}
static void check(const feat::MapEnum& g) {
    CHK(g.h == feat::Hue::H_BLUE, "mapenum.h");
    CHK(g.m.size() == 1 && g.m[0].first == 3 && g.m[0].second.x == 11 && g.m[0].second.y == 12, "mapenum.m");
    CHK(g.sels.size() == 1 && g.sels[0]._d() == 2 && g.sels[0].n() == 9, "mapenum.sels");
}
static void check(const feat::MapPrim& g) {
    CHK(g.m.size() == 2 && g.m[0] == std::make_pair(7, 42) && g.m[1] == std::make_pair(8, 99), "mapprim.m");
}
static void check(const feat::Money& g) { CHK(g.price == "123.45" && g.qty == "1234", "fixed"); }

// ---- drivers -----------------------------------------------------------------
static bool read_file(const std::string& path, std::vector<std::uint8_t>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return !out.empty();
}

template <class T> static int encode_to(const char* path, const T& v) {
    auto bytes = TS<T>::encode(v);
    std::ofstream f(path, std::ios::binary);
    if (!f) return 1;
    f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    std::printf("ENCODE OK: %zu bytes -> %s\n", bytes.size(), path);
    return 0;
}

template <class T> static void decode_check(const std::vector<std::uint8_t>& bytes, bool be) {
    try {
        check(TS<T>::decode(bytes.data(), bytes.size(), kEncoding, be));
    } catch (const std::exception& e) {
        ++g_fail;
        std::fprintf(stderr, "  [FAIL] decode: %s\n", e.what());
    }
}

static int decode_one(const std::string& feat, const std::vector<std::uint8_t>& b, bool be) {
    if (feat == "wstr") decode_check<feat::WStr>(b, be);
    else if (feat == "mut") decode_check<feat::Mut>(b, be);
    else if (feat == "mutnest") decode_check<feat::MutNest>(b, be);
    else if (feat == "outerkey") decode_check<feat::OuterKey>(b, be);
    else if (feat == "bits") decode_check<feat::Bits>(b, be);
    else if (feat == "tree") decode_check<feat::Tree>(b, be);
    else if (feat == "arr") decode_check<feat::Arr>(b, be);
    else if (feat == "prim") decode_check<feat::Prim>(b, be);
    else if (feat == "mapenum") decode_check<feat::MapEnum>(b, be);
    else if (feat == "mapprim") decode_check<feat::MapPrim>(b, be);
    else if (feat == "fixed") decode_check<feat::Money>(b, be);
    else { std::fprintf(stderr, "unknown feature %s\n", feat.c_str()); return 2; }
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 3) { std::fprintf(stderr, "usage: <feature> ENCODE|DECODE <file> | - BE\n"); return 2; }
    const std::string feat = argv[1];
    const std::string mode = argv[2];
    const char* file = argc > 3 ? argv[3] : nullptr;

    if (mode == "ENCODE") {
        if (!file) return 2;
        if (feat == "wstr") return encode_to(file, canon_wstr());
        if (feat == "mut") return encode_to(file, canon_mut());
        if (feat == "mutnest") return encode_to(file, canon_mutnest());
        if (feat == "outerkey") return encode_to(file, canon_outerkey());
        if (feat == "bits") return encode_to(file, canon_bits());
        if (feat == "tree") return encode_to(file, canon_tree());
        if (feat == "arr") return encode_to(file, canon_arr());
        if (feat == "prim") return encode_to(file, canon_prim());
        if (feat == "mapenum") return encode_to(file, canon_mapenum());
        if (feat == "mapprim") return encode_to(file, canon_mapprim());
        if (feat == "fixed") return encode_to(file, canon_fixed());
        std::fprintf(stderr, "unknown feature\n"); return 2;
    }
    if (mode == "DECODE") {
        std::vector<std::uint8_t> b;
        if (!file || !read_file(file, b)) { std::fprintf(stderr, "missing/empty %s\n", file ? file : "(none)"); return 2; }
        int rc = decode_one(feat, b, false);
        if (rc) return rc;
        if (g_fail == 0) { std::printf("DECODE OK: %s\n", feat.c_str()); return 0; }
        std::printf("DECODE FAILED: %d mismatch(es)\n", g_fail); return 1;
    }
    if (mode == "XCDR1") {
        // XCDR1 DECODE of the same Rust reference goldens, both byte orders.
        for (const char* f : {"wstr", "mut", "bits", "tree", "arr", "prim", "mapenum"}) {
            for (const char* tag : {"xcdr1-le", "xcdr1-be"}) {
                std::vector<std::uint8_t> b;
                std::string p = std::string("../../proofs/endianness/goldens/") + f + "." + tag + ".rust.bin";
                if (!read_file(p, b)) { std::printf("  %s %s: MISSING %s\n", f, tag, p.c_str()); ++g_fail; continue; }
                int before = g_fail;
                decode_one(f, b, std::string(tag) == "xcdr1-be");
                std::printf("  %s %s: %s\n", f, tag, g_fail == before ? "OK" : "FAIL");
            }
        }
        if (g_fail == 0) { std::printf("XCDR1 PASS\n"); return 0; }
        std::printf("XCDR1 FAILED: %d mismatch(es)\n", g_fail); return 1;
    }
    if (mode == "BE") {
        // Big-endian DECODE of the ZeroDDS-Rust reference big-endian wire
        // (proofs/endianness BE goldens — an independent encoder).
        for (const char* f : {"wstr", "mut", "bits", "tree", "arr", "prim", "mapenum"}) {
            std::vector<std::uint8_t> b;
            std::string p = std::string("../../proofs/endianness/goldens/") + f + ".xcdr2-be.rust.bin";
            if (!read_file(p, b)) { std::printf("  %s be: MISSING %s\n", f, p.c_str()); ++g_fail; continue; }
            int before = g_fail;
            decode_one(f, b, true);
            std::printf("  %s be: %s\n", f, g_fail == before ? "OK" : "FAIL");
        }
        if (g_fail == 0) { std::printf("BE/LE roundtrip PASS\n"); return 0; }
        std::printf("BE FAILED: %d mismatch(es)\n", g_fail); return 1;
    }
    std::fprintf(stderr, "bad args\n"); return 2;
}
