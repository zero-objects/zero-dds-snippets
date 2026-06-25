#include "MapBare.hpp"
#include <fastcdr/Cdr.h>
#include <fastcdr/FastBuffer.h>
#include <cstdio>
#include <cstring>
using namespace eprosima::fastcdr;
int main(){ char buf[256]; memset(buf,0,sizeof buf); FastBuffer fb(buf,sizeof buf);
  Cdr cdr(fb,Cdr::LITTLE_ENDIANNESS,CdrVersion::XCDRv2);
  MapBare s; s.m()[7]=42; serialize(cdr,s);
  size_t n=cdr.get_serialized_data_length();
  FILE*f=fopen("/root/xv-mapenum/mapbare.fd.bin","wb"); fwrite(buf,1,n,f); fclose(f);
  printf("FD len=%zu\n",n); return 0; }
