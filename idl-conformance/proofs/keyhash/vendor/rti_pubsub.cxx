#include <dds/dds.hpp>
#include <rti/config/Logger.hpp>
#include "rti_gen/keyhash_full.hpp"
#include <thread>
#include <chrono>
using namespace dds::core; using namespace dds::domain; using namespace dds::topic; using namespace dds::pub; using namespace dds::sub;

int main(){
  DomainParticipant pp(0);
  Topic<keyhash::SingleKey> ta(pp,"KH_SingleKey");
  Topic<keyhash::BigKey>    tb(pp,"KH_BigKey");
  Topic<keyhash::MixedKeys> tc(pp,"KH_MixedKeys");
  Topic<keyhash::OuterKey>  td(pp,"KH_OuterKey");
  Publisher pub(pp); Subscriber sub(pp);
  auto wq = dds::pub::qos::DataWriterQos{} << dds::core::policy::Reliability::Reliable() << dds::core::policy::Durability::TransientLocal();
  auto rq = dds::sub::qos::DataReaderQos{} << dds::core::policy::Reliability::Reliable() << dds::core::policy::Durability::TransientLocal();
  DataWriter<keyhash::SingleKey> wa(pub,ta,wq); DataReader<keyhash::SingleKey> ra(sub,ta,rq);
  DataWriter<keyhash::BigKey>    wb(pub,tb,wq); DataReader<keyhash::BigKey>    rb(sub,tb,rq);
  DataWriter<keyhash::MixedKeys> wc(pub,tc,wq); DataReader<keyhash::MixedKeys> rc(sub,tc,rq);
  DataWriter<keyhash::OuterKey>  wd(pub,td,wq); DataReader<keyhash::OuterKey>  rd(sub,td,rq);
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  keyhash::SingleKey a; a.id=0x11223344; a.payload=999;
  keyhash::BigKey b; b.name="sensor-array-north-wing"; b.region=7; b.payload=999;
  keyhash::MixedKeys c; c.a=0xAA; c.b=0x1234; c.c=0x55667788; c.d=(char)0x5A; c.payload=999;
  keyhash::NestedKey nk; nk.hi=0x01020304; nk.lo=0x05060708;
  keyhash::OuterKey d; d.k=nk; d.payload=999;
  wa.write(a); wb.write(b); wc.write(c); wd.write(d);
  printf("published\n"); fflush(stdout);
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  return 0;
}
