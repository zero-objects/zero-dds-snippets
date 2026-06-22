// Replicates the FastDDS-generated TopicDataType::compute_key key serialization
// using FastDDS own FastCDR (XCDRv2, BIG_ENDIANNESS, no encapsulation) and the
// FastDDS MD5 class, for the same instances as the ZeroDDS proof.
#include <fastcdr/Cdr.h>
#include <fastcdr/FastBuffer.h>
#include <fastdds/utils/md5.hpp>
#include <cstdio>
#include <cstring>
#include <string>
using namespace eprosima::fastcdr;

static void report(const char* label, const unsigned char* buf, size_t n, bool over16) {
  printf("%s key_len=%zu over16=%d\n", label, n, over16);
  printf("  raw_be   = "); for(size_t i=0;i<n;i++) printf("%02x", buf[i]); printf("\n");
  unsigned char k16[16]; memset(k16,0,16); memcpy(k16, buf, n<16?n:16);
  printf("  padded16 = "); for(int i=0;i<16;i++) printf("%02x", k16[i]); printf("\n");
  eprosima::fastdds::MD5 md5; md5.update(buf, (unsigned)n); md5.finalize();
  printf("  md5      = %s\n", md5.hexdigest().c_str());
}

template<class F>
static void run(const char* label, size_t maxsize, bool md5_branch, F serialize_key) {
  char* mem = new char[maxsize+16]; memset(mem,0,maxsize+16);
  FastBuffer fb(mem, maxsize+16);
  Cdr ser(fb, Cdr::BIG_ENDIANNESS, CdrVersion::XCDRv2);
  serialize_key(ser);
  size_t n = ser.get_serialized_data_length();
  report(label, (const unsigned char*)mem, n, md5_branch);
  delete[] mem;
}

int main(){
  // (a) SingleKey: @key long id=0x11223344
  run("(a) SingleKey", 64, false, [](Cdr& s){ int32_t id=0x11223344; s.serialize(id); });
  // (b) BigKey: @key string name + @key long region. unbounded string -> md5.
  run("(b) BigKey   ", 256, true, [](Cdr& s){ std::string name="sensor-array-north-wing"; int32_t region=7; s.serialize(name); s.serialize(region); });
  // (c) MixedKeys: octet a, short b, long c, char d
  run("(c) MixedKeys", 64, false, [](Cdr& s){ uint8_t a=0xAA; int16_t b=0x1234; int32_t c=0x55667788; char d=(char)0x5A; s.serialize(a); s.serialize(b); s.serialize(c); s.serialize(d); });
  // (d) OuterKey: nested NestedKey{ hi, lo } both @key long
  run("(d) OuterKey ", 64, false, [](Cdr& s){ int32_t hi=0x01020304, lo=0x05060708; s.serialize(hi); s.serialize(lo); });
  return 0;
}
