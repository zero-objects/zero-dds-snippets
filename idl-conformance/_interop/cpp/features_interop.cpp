// idl-conformance / _interop / cpp — per-feature XCDR2-LE golden producer.
//
//   <feature> ENCODE <out.bin>   -> write canonical feat::<T> as raw XCDR2-LE
//   <feature> DECODE <in.bin>    -> decode, assert == canonical, exit 0/1
//
// feature ∈ wstr|mut|bits|tree|arr|prim. Values are the CANONICAL.md samples.
#include "gen_feat/features.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace x = dds::topic::xcdr2;
template <class T> using TS = dds::topic::topic_type_support<T>;

static int g_fail = 0;
template <typename A, typename B>
static void chk(const char* what, const A& got, const B& want) {
    if (!(got == want)) { ++g_fail; std::cout << "  [FAIL] " << what << "\n"; }
}

// ---- canonical samples (CANONICAL.md) ----------------------------------
static feat::WStr canon_wstr() {
    feat::WStr v;
    v.label(std::wstring{L'c', L'a', L'f', wchar_t(0x00E9)});            // café
    v.text(std::wstring{wchar_t(0x65E5), wchar_t(0x672C), wchar_t(0x8A9E),
                        wchar_t(0x1F389)});                              // 日本語🎉
    return v;
}
static feat::Mut canon_mut() {
    feat::Mut v; v.a(1000000); v.b(2.5); v.c("ok"); return v;
}
static feat::Bits canon_bits() {
    feat::Bits v;
    v.perm(::feat::Perm(0x05));    // READ|EXEC
    ::feat::Flags fl; fl.kind(5); fl.prio(20); v.flags(fl);
    return v;
}
static feat::Tree canon_tree() {
    feat::Tree root; root.value(1);
    feat::Tree k0; k0.value(2);
    feat::Tree gc; gc.value(4);
    k0.kids(std::vector<feat::Tree>{gc});
    feat::Tree k1; k1.value(3);
    root.kids(std::vector<feat::Tree>{k0, k1});
    return root;
}
static feat::Arr canon_arr() {
    feat::Arr v;
    std::array<std::array<int32_t,3>,2> g{{{{10,11,12}},{{13,14,15}}}};
    v.grid(g);
    feat::Pt p0; p0.x(1); p0.y(2);
    feat::Pt p1; p1.x(3); p1.y(4);
    std::array<feat::Pt,2> sh{{p0,p1}};
    v.shape(sh);
    return v;
}
static feat::Prim canon_prim() {
    feat::Prim v;
    v.i8(-128); v.u8(255); v.i16(-32768); v.u16(65535);
    v.i32(-2147483648); v.u32(4294967295u);
    v.i64(INT64_MIN); v.u64(UINT64_MAX);
    v.f32(3.5f); v.f64(-1234.5); v.b(true); v.o(0xAB); v.ch('Z');
    return v;
}
// nested @mutable (mutable-in-mutable + sequence<@mutable>): EMHEADER LC=5
// reuse — leaf/list members each begin with a DHEADER that doubles as NEXTINT.
static feat::MutNest canon_mutnest() {
    feat::MutNest v;
    v.tag(9);
    feat::MutLeaf leaf; leaf.u(100); leaf.v(1.25); v.leaf(leaf);
    feat::MutLeaf e0; e0.u(1); e0.v(0.5);
    feat::MutLeaf e1; e1.u(2); e1.v(0.25);
    v.list(std::vector<feat::MutLeaf>{e0, e1});
    return v;
}
// nested-struct @key (@final outer + @nested @final NestedKey): plain flat
// XCDR2 (no EMHEADER); the @key only drives the keyhash, not these wire bytes.
static feat::OuterKey canon_outerkey() {
    feat::OuterKey v;
    feat::NestedKey k; k.hi(0x01020304); k.lo(0x05060708); v.k(k);
    v.payload(999);
    return v;
}
// @bit_bound(16) enum (2-byte holder) + map<long,Pt> (map of struct) +
// sequence<Sel> (sequence of union). h=H_BLUE(2); m={3:{11,12}}; sels=[Sel::n=9].
static feat::MapEnum canon_mapenum() {
    feat::MapEnum v;
    v.h(feat::Hue::H_BLUE);
    feat::Pt p; p.x(11); p.y(12);
    std::map<int32_t, feat::Pt> m; m[3] = p; v.m(m);
    feat::Sel s; s._d(2); s.value() = static_cast<int32_t>(9);
    v.sels(std::vector<feat::Sel>{s});
    return v;
}

static std::vector<uint8_t> readfile(const char* p) {
    std::ifstream f(p, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                                std::istreambuf_iterator<char>());
}
static int writefile(const char* p, const std::vector<uint8_t>& b) {
    std::ofstream f(p, std::ios::binary);
    if (!f) return 1;
    f.write(reinterpret_cast<const char*>(b.data()), b.size());
    return f ? 0 : 1;
}

template <class T>
static int do_encode(const char* path, const T& v) {
    auto wire = TS<T>::encode(v, x::XcdrVersion::Xcdr2);
    if (writefile(path, wire)) { std::cerr << "write fail\n"; return 2; }
    std::cout << "ENCODE OK: " << wire.size() << " bytes -> " << path << "\n";
    return 0;
}

static void verify_wstr(const feat::WStr& g, const feat::WStr& w) {
    chk("label", g.label(), w.label());
    chk("text", g.text(), w.text());
}
static void verify_mut(const feat::Mut& g, const feat::Mut& w) {
    chk("a", g.a(), w.a()); chk("b", g.b(), w.b()); chk("c", g.c(), w.c());
}
static void verify_bits(const feat::Bits& g, const feat::Bits& w) {
    chk("perm", uint32_t(g.perm()), uint32_t(w.perm()));
    chk("flags.kind", uint32_t(g.flags().kind()), uint32_t(w.flags().kind()));
    chk("flags.prio", uint32_t(g.flags().prio()), uint32_t(w.flags().prio()));
}
static void verify_tree(const feat::Tree& g, const feat::Tree& w) {
    chk("value", g.value(), w.value());
    chk("kids.size", g.kids().size(), w.kids().size());
    if (g.kids().size() == w.kids().size())
        for (size_t i = 0; i < g.kids().size(); ++i) verify_tree(g.kids()[i], w.kids()[i]);
}
static void verify_arr(const feat::Arr& g, const feat::Arr& w) {
    for (int i = 0; i < 2; ++i) for (int j = 0; j < 3; ++j)
        chk("grid", g.grid()[i][j], w.grid()[i][j]);
    for (int i = 0; i < 2; ++i) {
        chk("shape.x", g.shape()[i].x(), w.shape()[i].x());
        chk("shape.y", g.shape()[i].y(), w.shape()[i].y());
    }
}
static void verify_prim(const feat::Prim& g, const feat::Prim& w) {
    chk("i8", g.i8(), w.i8()); chk("u8", g.u8(), w.u8());
    chk("i16", g.i16(), w.i16()); chk("u16", g.u16(), w.u16());
    chk("i32", g.i32(), w.i32()); chk("u32", g.u32(), w.u32());
    chk("i64", g.i64(), w.i64()); chk("u64", g.u64(), w.u64());
    chk("f32", g.f32(), w.f32()); chk("f64", g.f64(), w.f64());
    chk("b", g.b(), w.b()); chk("o", uint32_t(g.o()), uint32_t(w.o()));
    chk("ch", g.ch(), w.ch());
}
static void verify_mutnest(const feat::MutNest& g, const feat::MutNest& w) {
    chk("tag", g.tag(), w.tag());
    chk("leaf.u", g.leaf().u(), w.leaf().u());
    chk("leaf.v", g.leaf().v(), w.leaf().v());
    chk("list.size", g.list().size(), w.list().size());
    if (g.list().size() == w.list().size())
        for (size_t i = 0; i < g.list().size(); ++i) {
            chk("list.u", g.list()[i].u(), w.list()[i].u());
            chk("list.v", g.list()[i].v(), w.list()[i].v());
        }
}
static void verify_outerkey(const feat::OuterKey& g, const feat::OuterKey& w) {
    chk("k.hi", g.k().hi(), w.k().hi());
    chk("k.lo", g.k().lo(), w.k().lo());
    chk("payload", g.payload(), w.payload());
}

int main(int argc, char** argv) {
    if (argc < 3) { std::cerr << "usage: <feature> ENCODE|DECODE <file>\n"; return 2; }
    std::string feat = argv[1], mode = argv[2];
    const char* file = argc > 3 ? argv[3] : nullptr;

    if (mode == "ENCODE") {
        if (feat == "wstr") return do_encode(file, canon_wstr());
        if (feat == "mut")  return do_encode(file, canon_mut());
        if (feat == "bits") return do_encode(file, canon_bits());
        if (feat == "tree") return do_encode(file, canon_tree());
        if (feat == "arr")  return do_encode(file, canon_arr());
        if (feat == "prim") return do_encode(file, canon_prim());
        if (feat == "mutnest")  return do_encode(file, canon_mutnest());
        if (feat == "outerkey") return do_encode(file, canon_outerkey());
        if (feat == "mapenum")  return do_encode(file, canon_mapenum());
    } else if (mode == "DECODE") {
        auto buf = readfile(file);
        if (buf.empty()) { std::cerr << "empty/missing " << file << "\n"; return 2; }
        if (feat == "wstr") verify_wstr(TS<feat::WStr>::decode(buf.data(), buf.size(), x::XcdrVersion::Xcdr2), canon_wstr());
        else if (feat == "mut") verify_mut(TS<feat::Mut>::decode(buf.data(), buf.size(), x::XcdrVersion::Xcdr2), canon_mut());
        else if (feat == "bits") verify_bits(TS<feat::Bits>::decode(buf.data(), buf.size(), x::XcdrVersion::Xcdr2), canon_bits());
        else if (feat == "tree") verify_tree(TS<feat::Tree>::decode(buf.data(), buf.size(), x::XcdrVersion::Xcdr2), canon_tree());
        else if (feat == "arr") verify_arr(TS<feat::Arr>::decode(buf.data(), buf.size(), x::XcdrVersion::Xcdr2), canon_arr());
        else if (feat == "prim") verify_prim(TS<feat::Prim>::decode(buf.data(), buf.size(), x::XcdrVersion::Xcdr2), canon_prim());
        else if (feat == "mutnest") verify_mutnest(TS<feat::MutNest>::decode(buf.data(), buf.size(), x::XcdrVersion::Xcdr2), canon_mutnest());
        else if (feat == "outerkey") verify_outerkey(TS<feat::OuterKey>::decode(buf.data(), buf.size(), x::XcdrVersion::Xcdr2), canon_outerkey());
        else if (feat == "mapenum") { auto v = TS<feat::MapEnum>::decode(buf.data(), buf.size(), x::XcdrVersion::Xcdr2); auto re = TS<feat::MapEnum>::encode(v, x::XcdrVersion::Xcdr2); if (re != buf) { g_fail++; std::cerr << "  [FAIL] mapenum roundtrip re-encode != golden\n"; } }
        else { std::cerr << "unknown feature\n"; return 2; }
        if (g_fail == 0) { std::cout << "DECODE OK: " << feat << "\n"; return 0; }
        std::cout << "DECODE FAILED: " << g_fail << " mismatch(es)\n"; return 1;
    }
    std::cerr << "bad args\n"; return 2;
}
