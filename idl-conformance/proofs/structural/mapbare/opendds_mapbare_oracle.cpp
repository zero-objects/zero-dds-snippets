#include "MapBareTypeSupportImpl.h"
#include <dds/DCPS/Serializer.h>
#include <fstream>
int main(){ MapBare s; s.m()[7]=42;
  OpenDDS::DCPS::Encoding enc(OpenDDS::DCPS::Encoding::KIND_XCDR2, OpenDDS::DCPS::ENDIAN_LITTLE);
  size_t sz=OpenDDS::DCPS::serialized_size(enc,s); ACE_Message_Block mb(sz);
  OpenDDS::DCPS::Serializer ser(&mb,enc); ser<<s;
  std::ofstream f("/root/xv-mapenum/mapbare.od.bin",std::ios::binary); f.write(mb.rd_ptr(),mb.length());
  return 0; }
