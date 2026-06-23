// FastCDR/FastDDS vendor oracle: serialize the CANONICAL.md samples in all four
// representations (XCDR2-LE, XCDR2-BE, XCDR1-LE, XCDR1-BE) using the
// fastddsgen-generated serialize() functions, and dump the BARE CDR body
// (no encapsulation header) per (feature, repr).
#include "gen/features_no_tree.hpp"
#include <fastcdr/Cdr.h>
#include <fastcdr/FastBuffer.h>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>

using eprosima::fastcdr::Cdr;
using eprosima::fastcdr::FastBuffer;
using eprosima::fastcdr::CdrVersion;

struct Repr { const char* tag; Cdr::Endianness endian; CdrVersion ver; };
static const Repr REPRS[4] = {
  {"xcdr2-le", Cdr::LITTLE_ENDIANNESS, CdrVersion::XCDRv2},
  {"xcdr2-be", Cdr::BIG_ENDIANNESS,    CdrVersion::XCDRv2},
  {"xcdr1-le", Cdr::LITTLE_ENDIANNESS, CdrVersion::XCDRv1},
  {"xcdr1-be", Cdr::BIG_ENDIANNESS,    CdrVersion::XCDRv1},
};

template <typename T>
void emit(const char* name, const T& sample) {
  for (const auto& r : REPRS) {
    char buf[512]; memset(buf, 0, sizeof(buf));
    FastBuffer fb(buf, sizeof(buf));
    Cdr cdr(fb, r.endian, r.ver);
    // NO serialize_encapsulation(): we want the bare body.
    eprosima::fastcdr::serialize(cdr, sample);
    size_t len = cdr.get_serialized_data_length();
    char path[256];
    snprintf(path, sizeof(path), "/tmp/oracle-endian/out/%s.%s.fastdds.bin", name, r.tag);
    FILE* f = fopen(path, "wb");
    fwrite(buf, 1, len, f);
    fclose(f);
    printf("FASTDDS %-5s %-8s len=%zu  ->  %s\n", name, r.tag, len, path);
  }
}

int main() {
  // wstr
  {
    feat::WStr s;
    s.label(std::wstring(L"café"));
    s.text(std::wstring(L"日本語\U0001F389"));
    emit("wstr", s);
  }
  // mut
  {
    feat::Mut s; s.a(1000000); s.b(2.5); s.c(std::string("ok"));
    emit("mut", s);
  }
  // bits
  {
    feat::Bits s;
    s.perm(feat::READ | feat::EXEC);
    feat::Flags fl; fl.kind = 5; fl.prio = 20; s.flags(fl);
    emit("bits", s);
  }
  // arr
  {
    feat::Arr s;
    std::array<std::array<int32_t,3>,2> g = {{ {{10,11,12}}, {{13,14,15}} }};
    s.grid(g);
    std::array<feat::Pt,2> sh;
    sh[0].x(1); sh[0].y(2); sh[1].x(3); sh[1].y(4);
    s.shape(sh);
    emit("arr", s);
  }
  // prim
  {
    feat::Prim s;
    s.i8(-128); s.u8(255); s.i16(-32768); s.u16(65535);
    s.i32(-2147483648); s.u32(4294967295u);
    s.i64(INT64_MIN); s.u64(UINT64_MAX);
    s.f32(3.5f); s.f64(-1234.5); s.b(true); s.o(0xAB); s.ch('Z');
    emit("prim", s);
  }
  // mapenum: @bit_bound(16) enum + map<long,Pt> + sequence<Sel>
  {
    feat::MapEnum s;
    s.h(feat::Hue::H_BLUE);
    std::map<int32_t, feat::Pt> m;
    feat::Pt p; p.x(11); p.y(12);
    m[3] = p;
    s.m(m);
    std::vector<feat::Sel> v;
    feat::Sel sel; sel.n(9);   // case 2 — the setter also sets the discriminator
    v.push_back(sel);
    s.sels(v);
    emit("mapenum", s);
  }
  return 0;
}
