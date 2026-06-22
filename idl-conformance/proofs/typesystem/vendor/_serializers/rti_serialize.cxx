// Native RTI Connext 7.7 XCDR2-LE serializer for the wave-2 corpus.
// Uses rti::topic::to_cdr_buffer with the XCDR2 representation, then strips the
// 4-byte CDR encapsulation header to leave the bare XCDR2 payload.
#include "typesystem_rti.hpp"
#include <rti/topic/cdr.hpp>

#include <cstdio>
#include <fstream>
#include <vector>

template <typename T>
static void emit(const char* name, const T& sample)
{
    std::vector<char> buf;
    rti::topic::to_cdr_buffer(
        buf, sample, dds::core::policy::DataRepresentation::xcdr2());

    // Strip the 4-byte encapsulation header (id + options).
    std::vector<uint8_t> payload(buf.begin() + 4, buf.end());

    std::string path = std::string("/tmp/oracle-typesys/out/") + name + ".rti.bin";
    std::ofstream o(path, std::ios::binary);
    o.write(reinterpret_cast<const char*>(payload.data()), payload.size());
    o.close();

    printf("%-20s %zu bytes:", name, payload.size());
    for (auto b : payload) printf(" %02x", b);
    printf("\n");
}

int main()
{
    using namespace ts;

    // enum_holder: big=BLUE (aggregate field)
    {
        EnumHolder h;
        h.big = Color::BLUE;
        emit("enum_holder", h);
    }
    // union_long_default: note="hi" with default-selecting discriminator (0)
    {
        LongUnion u;
        u.note(std::string("hi"), 0);
        emit("union_long_default", u);
    }
    // union_long_struct: where={7,8}
    {
        LongUnion u;
        Point p; p.x = 7; p.y = 8;
        u.where(p);
        emit("union_long_struct", u);
    }
    // union_enum_default: fallback=42 (disc BLUE)
    {
        EnumUnion u;
        u.fallback(42, Color::BLUE);
        emit("union_enum_default", u);
    }
    // opt_holder: id=1, maybe_num=77, maybe_str=absent, maybe_obj={5,6}
    {
        OptHolder h;
        h.id = 1;
        h.maybe_num = 77;
        Inner in; in.a = 5; in.b = 6;
        h.maybe_obj = in;
        emit("opt_holder", h);
    }
    // mut_outer: tag=9, leaf={100,1.25}, list=[{1,0.5},{2,0.25}]
    {
        MutOuter o;
        o.tag = 9;
        MutLeaf leaf; leaf.u = 100; leaf.v = 1.25;
        o.leaf = leaf;
        std::vector<MutLeaf> list;
        MutLeaf l1; l1.u = 1; l1.v = 0.5;
        MutLeaf l2; l2.u = 2; l2.v = 0.25;
        list.push_back(l1); list.push_back(l2);
        o.list = list;
        emit("mut_outer", o);
    }
    return 0;
}
