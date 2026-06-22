// FastDDS TypeObject / TypeIdentifier extractor.
//
// Registers each canonical type via the generated register_*_type_identifier()
// functions, then for each type:
//   * fetches the Minimal + Complete TypeObject from the TypeObjectRegistry,
//   * serializes it with fastcdr in XCDR encoding v2, Little-Endian
//     (XTypes 1.3 §7.3.4.5), wrapped so the bytes are bit-identical to what
//     FastDDS hashes,
//   * pulls the 14-byte EquivalenceHash from the registered TypeIdentifier.
//
// Writes <type>.{minimal,complete}.bin (serialized TypeObject) and
// <type>.{minimal,complete}.hash (28 hex chars) into the directory given by
// argv[1].
//
// Build (codepit):
//   g++ -std=c++17 fastdds_extract.cpp \
//       /tmp/oracle-typeobj/typeobjectTypeObjectSupport.cxx \
//       -I/tmp/oracle-typeobj -I/opt/fastdds/include \
//       -I/opt/fastcdr/include -L/opt/fastdds/lib -L/opt/fastcdr/lib \
//       -lfastdds -lfastcdr -o /tmp/oracle-typeobj/fastdds_extract

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/xtypes/type_representation/TypeObject.hpp>
#include <fastdds/dds/xtypes/type_representation/ITypeObjectRegistry.hpp>

#include <fastcdr/Cdr.h>
#include <fastcdr/FastBuffer.h>

#include "typeobjectTypeObjectSupport.hpp"

using namespace eprosima::fastdds::dds::xtypes;
using eprosima::fastdds::dds::DomainParticipantFactory;
using eprosima::fastdds::dds::RETCODE_OK;

// Registration entrypoints are declared in typeobjectTypeObjectSupport.hpp
// inside namespace `to`.

static std::string g_dir;

static std::string hex14(const EquivalenceHash& h)
{
    static const char* X = "0123456789abcdef";
    std::string s;
    for (int i = 0; i < 14; ++i)
    {
        unsigned char b = h[i];
        s.push_back(X[b >> 4]);
        s.push_back(X[b & 0xf]);
    }
    return s;
}

template <typename T>
static std::vector<unsigned char> serialize_xcdr2(const T& obj)
{
    // Pre-flight to size the buffer, then serialize XCDR v2 LE. We capture the
    // serialized TypeObject body exactly as the registry would hash it.
    eprosima::fastcdr::CdrSizeCalculator calc(
        eprosima::fastcdr::CdrVersion::XCDRv2);
    size_t cur = 0;
    size_t sz = calc.calculate_serialized_size(obj, cur) + 8;
    std::vector<unsigned char> buf(sz);
    eprosima::fastcdr::FastBuffer fb(reinterpret_cast<char*>(buf.data()), sz);
    eprosima::fastcdr::Cdr cdr(fb, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS,
            eprosima::fastcdr::CdrVersion::XCDRv2);
    cdr.serialize_encapsulation();           // we strip this below
    size_t enc = cdr.get_serialized_data_length();
    cdr << obj;
    size_t total = cdr.get_serialized_data_length();
    // Strip the 4-byte encapsulation header so the bytes start at the
    // TypeObject body, matching the cross-vendor framing rule documented in
    // README.md.
    return std::vector<unsigned char>(buf.begin() + enc, buf.begin() + total);
}

static void write_file(const std::string& name, const void* data, size_t n)
{
    std::string p = g_dir + "/" + name;
    FILE* f = std::fopen(p.c_str(), "wb");
    if (f) { std::fwrite(data, 1, n, f); std::fclose(f); }
}

static void dump(const char* case_name, const std::string& type_name)
{
    auto& reg = DomainParticipantFactory::get_instance()->type_object_registry();

    TypeIdentifierPair ids;
    if (RETCODE_OK != reg.get_type_identifiers(type_name, ids))
    {
        std::fprintf(stderr, "%s: get_type_identifiers FAILED\n", case_name);
        return;
    }
    TypeObjectPair objs;
    if (RETCODE_OK != reg.get_type_objects(type_name, objs))
    {
        std::fprintf(stderr, "%s: get_type_objects FAILED\n", case_name);
        return;
    }

    // Minimal
    {
        auto bytes = serialize_xcdr2(objs.minimal_type_object);
        write_file(std::string(case_name) + ".minimal.bin", bytes.data(), bytes.size());
        // The minimal TypeIdentifier carries the EK_MINIMAL hash.
        if (ids.type_identifier1()._d() == EK_MINIMAL)
        {
            auto h = hex14(ids.type_identifier1().equivalence_hash());
            write_file(std::string(case_name) + ".minimal.hash", h.data(), h.size());
            std::printf("%-14s MINIMAL  len=%3zu hash=%s tk=0x%02x\n",
                    case_name, bytes.size(), h.c_str(), bytes.size() ? bytes[1] : 0);
        }
        else if (ids.type_identifier2()._d() == EK_MINIMAL)
        {
            auto h = hex14(ids.type_identifier2().equivalence_hash());
            write_file(std::string(case_name) + ".minimal.hash", h.data(), h.size());
            std::printf("%-14s MINIMAL  len=%3zu hash=%s tk=0x%02x\n",
                    case_name, bytes.size(), h.c_str(), bytes.size() ? bytes[1] : 0);
        }
    }
    // Complete
    {
        auto bytes = serialize_xcdr2(objs.complete_type_object);
        write_file(std::string(case_name) + ".complete.bin", bytes.data(), bytes.size());
        if (ids.type_identifier1()._d() == EK_COMPLETE)
        {
            auto h = hex14(ids.type_identifier1().equivalence_hash());
            write_file(std::string(case_name) + ".complete.hash", h.data(), h.size());
            std::printf("%-14s COMPLETE len=%3zu hash=%s tk=0x%02x\n",
                    case_name, bytes.size(), h.c_str(), bytes.size() ? bytes[1] : 0);
        }
        else if (ids.type_identifier2()._d() == EK_COMPLETE)
        {
            auto h = hex14(ids.type_identifier2().equivalence_hash());
            write_file(std::string(case_name) + ".complete.hash", h.data(), h.size());
            std::printf("%-14s COMPLETE len=%3zu hash=%s tk=0x%02x\n",
                    case_name, bytes.size(), h.c_str(), bytes.size() ? bytes[1] : 0);
        }
    }
}

int main(int argc, char** argv)
{
    g_dir = (argc > 1) ? argv[1] : ".";

    TypeIdentifierPair t;
    to::register_Plain_type_identifier(t);
    to::register_Appendable_type_identifier(t);
    to::register_Mutable_type_identifier(t);
    to::register_Color_type_identifier(t);
    to::register_LongSeq4_type_identifier(t);
    to::register_Inner_type_identifier(t);
    to::register_Nested_type_identifier(t);
    to::register_Choice_type_identifier(t);

    dump("plain",           "to::Plain");
    dump("appendable",      "to::Appendable");
    dump("mutable",         "to::Mutable");
    dump("color_enum",      "to::Color");
    dump("long_seq4_alias", "to::LongSeq4");
    dump("inner",           "to::Inner");
    dump("nested",          "to::Nested");
    dump("choice_union",    "to::Choice");
    return 0;
}
