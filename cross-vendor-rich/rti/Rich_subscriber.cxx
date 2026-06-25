#include <dds/dds.hpp>
#include "Rich.hpp"
#include <thread>
#include <iostream>
#include <string>
template<class T>
int run(int dom, const std::string& tn){
  dds::domain::DomainParticipant p(dom);
  dds::topic::Topic<T> topic(p, tn);
  dds::sub::qos::DataReaderQos rq = dds::sub::Subscriber(p).default_datareader_qos();
  rq << dds::core::policy::Reliability::BestEffort();
  std::vector<dds::core::policy::DataRepresentationId> reps; reps.push_back(0); reps.push_back(2);
  rq << dds::core::policy::DataRepresentation(reps);
  dds::sub::DataReader<T> r(dds::sub::Subscriber(p), topic, rq);
  std::cout<<"[rti-sub] topic="<<tn<<std::endl;
  for(;;){
    auto samples = r.take();
    for(const auto& s : samples){ if(!s.info().valid()) continue; const auto& d=s.data();
      bool ok=true; for(size_t k=0;k<d.blob.size();k++) if(d.blob[k]!=(uint8_t)(k%251)){ok=false;break;}
      std::cout<<"  <- id="<<d.id<<" value="<<d.value<<" blob="<<d.blob.size()<<" integrity="<<(ok?"OK":"CORRUPT")<<std::endl; }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
int main(int argc, char** argv){
  int dom = argc>1?atoi(argv[1]):0; std::string e=argc>2?argv[2]:"m";
  if(e=="f") return run<RichF>(dom,"RichF");
  if(e=="a") return run<RichA>(dom,"RichA");
  return run<RichM>(dom,"RichM");
}
