#include "MapEnum.hpp"
#include <fastcdr/Cdr.h>
#include <fastcdr/FastBuffer.h>
#include <cstdio>
#include <cstring>
using eprosima::fastcdr::Cdr; using eprosima::fastcdr::FastBuffer; using eprosima::fastcdr::CdrVersion;
template<typename T> void emit(const char* name, const char* tag, Cdr::Endianness e, CdrVersion v, const T& s){
  char buf[1024]; memset(buf,0,sizeof buf); FastBuffer fb(buf,sizeof buf); Cdr cdr(fb,e,v);
  eprosima::fastcdr::serialize(cdr,s);
  size_t len=cdr.get_serialized_data_length();
  char p[256]; snprintf(p,sizeof p,"out/%s.%s.fastdds.bin",name,tag);
  FILE*f=fopen(p,"wb"); fwrite(buf,1,len,f); fclose(f);
  printf("FASTDDS %-7s %-8s len=%zu\n",name,tag,len);
}
int main(){
  feat::MapEnum s;
  s.h(feat::Hue::H_BLUE);
  feat::Pt pt; pt.x(11); pt.y(12);
  s.m()[3] = pt;
  feat::Sel sel; sel.n(9);
  s.sels().push_back(sel);
  emit("mapenum","xcdr2-le",Cdr::LITTLE_ENDIANNESS,CdrVersion::XCDRv2,s);
  emit("mapenum","xcdr2-be",Cdr::BIG_ENDIANNESS,CdrVersion::XCDRv2,s);
  return 0;
}
