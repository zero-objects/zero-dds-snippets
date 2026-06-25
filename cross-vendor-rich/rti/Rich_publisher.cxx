#include <dds/dds.hpp>
#include "Rich.hpp"
#include <thread>
#include <iostream>
#include <string>
template<class T>
int run(int dom, const std::string& tn, int n){
  dds::domain::DomainParticipant p(dom);
  dds::topic::Topic<T> topic(p, tn);
  dds::pub::DataWriter<T> w(dds::pub::Publisher(p), topic);
  for(int i=0;i<100 && dds::pub::matched_subscriptions(w).empty(); i++) std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::cout<<"[rti-pub] topic="<<tn<<" blob="<<n<<std::endl;
  for(uint32_t id=1;;id++){
    T s; s.id = id; s.value = 3.141592653589793*id;
    s.blob.resize(n); for(int k=0;k<n;k++) s.blob[k]=(uint8_t)(k%251);
    w.write(s); std::cout<<"  -> id="<<id<<std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
  }
}
int main(int argc, char** argv){
  int dom = argc>1?atoi(argv[1]):0; std::string e=argc>2?argv[2]:"m"; int n=argc>3?atoi(argv[3]):50;
  if(e=="f") return run<RichF>(dom,"RichF",n);
  if(e=="a") return run<RichA>(dom,"RichA",n);
  return run<RichM>(dom,"RichM",n);
}
