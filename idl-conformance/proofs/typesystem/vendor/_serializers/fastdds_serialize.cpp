// Native FastDDS/FastCDR XCDR2-LE serializer for the wave-2 type-system corpus.
// Emits the raw XCDR2 payload for each canonical sample (NO RTPS encapsulation
// header) to /tmp/oracle-typesys/out/<case>.fastdds.bin.
#include "typesystem.hpp"
#include "typesystemCdrAux.hpp"
#include "typesystemCdrAux.ipp"

#include <fastcdr/Cdr.h>
#include <fastcdr/FastBuffer.h>

#include <cstdio>
#include <fstream>
#include <vector>

using namespace eprosima::fastcdr;
using namespace ts;

template <typename T>
static std::vector<uint8_t> enc_xcdr2(const T& v)
{
    // Big enough buffer.
    char buf[4096];
    FastBuffer fb(buf, sizeof(buf));
    Cdr cdr(fb, Cdr::DEFAULT_ENDIAN, CdrVersion::XCDRv2);
    // Do NOT call serialize_encapsulation -> we want the bare payload.
    cdr << v;
    size_t len = cdr.get_serialized_data_length();
    return std::vector<uint8_t>(reinterpret_cast<uint8_t*>(buf),
                                reinterpret_cast<uint8_t*>(buf) + len);
}

static void dump(const char* name, const std::vector<uint8_t>& b)
{
    std::string path = std::string("/tmp/oracle-typesys/out/") + name + ".fastdds.bin";
    std::ofstream o(path, std::ios::binary);
    o.write(reinterpret_cast<const char*>(b.data()), b.size());
    o.close();
    printf("%-20s %zu bytes:", name, b.size());
    for (auto x : b) printf(" %02x", x);
    printf("\n");
}

int main()
{
    // enum_holder: big=BLUE, small=T_C, medium=M_D
    {
        EnumHolder h;
        h.big(Color::BLUE);
        h.small(Tiny::T_C);
        h.medium(Mid::MID_D);
        dump("enum_holder", enc_xcdr2(h));
    }
    // union_long_default: note="hi"
    {
        LongUnion u;
        u.note("hi");
        dump("union_long_default", enc_xcdr2(u));
    }
    // union_long_struct: where={7,8}
    {
        LongUnion u;
        Point p; p.x(7); p.y(8);
        u.where(p);
        dump("union_long_struct", enc_xcdr2(u));
    }
    // union_enum_default: fallback=42 (disc BLUE)
    {
        EnumUnion u;
        u.fallback(42);
        dump("union_enum_default", enc_xcdr2(u));
    }
    // opt_holder: id=1, maybe_num=77, maybe_str=absent, maybe_obj={5,6}
    {
        OptHolder h;
        h.id(1);
        h.maybe_num(77);
        // maybe_str left unset (nullopt)
        Inner in; in.a(5); in.b(6);
        h.maybe_obj(in);
        dump("opt_holder", enc_xcdr2(h));
    }
    // mut_outer: tag=9, leaf={100,1.25}, list=[{1,0.5},{2,0.25}]
    {
        MutOuter o;
        o.tag(9);
        MutLeaf leaf; leaf.u(100); leaf.v(1.25);
        o.leaf(leaf);
        std::vector<MutLeaf> list;
        MutLeaf l1; l1.u(1); l1.v(0.5);
        MutLeaf l2; l2.u(2); l2.v(0.25);
        list.push_back(l1); list.push_back(l2);
        o.list(list);
        dump("mut_outer", enc_xcdr2(o));
    }
    return 0;
}
