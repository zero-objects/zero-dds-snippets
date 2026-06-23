#include "arr.hpp"
#include <rti/topic/cdr.hpp>
#include <fstream>
#include <vector>
#include <cstdio>
int main() {
  feat::Arr a;
  int idx=0;
  for (int i=0;i<2;i++) for(int j=0;j<3;j++) a.grid[i][j]=10+idx++;
  a.shape[0].x=1; a.shape[0].y=2;
  a.shape[1].x=3; a.shape[1].y=4;
  std::vector<char> buf;
  rti::topic::to_cdr_buffer(buf, a, dds::core::policy::DataRepresentation::xcdr2());
  std::ofstream f("/root/sx2-oracle/arr.xcdr2-le.rti.bin", std::ios::binary);
  f.write(buf.data()+4, buf.size()-4);
  printf("RTI arr: total=%zu payload=%zu\n", buf.size(), buf.size()-4);
  return 0;
}
