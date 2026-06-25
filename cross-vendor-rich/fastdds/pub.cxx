#include "RichPubSubTypes.hpp"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/core/policy/QosPolicies.hpp>
#include <thread>
#include <string>
#include <iostream>
using namespace eprosima::fastdds::dds;
template<class T, class TS>
int run(DomainParticipant* p, const char* topic, int n){
  TypeSupport ts(new TS()); ts.register_type(p);
  Topic* tp = p->create_topic(topic, ts.get_type_name(), TOPIC_QOS_DEFAULT);
  Publisher* pub = p->create_publisher(PUBLISHER_QOS_DEFAULT);
  DataWriterQos wq = DATAWRITER_QOS_DEFAULT; wq.reliability().kind = RELIABLE_RELIABILITY_QOS;
  DataWriter* w = pub->create_datawriter(tp, wq);
  std::cout<<"[fd-pub] topic="<<topic<<" blob="<<n<<std::endl;
  for(int i=0;i<100;i++){ PublicationMatchedStatus s; w->get_publication_matched_status(s); if(s.current_count>=1)break; std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
  for(uint32_t id=1;;id++){
    T r; r.id(id); r.value(3.141592653589793*id);
    std::vector<uint8_t> b(n); for(int k=0;k<n;k++) b[k]=k%251; r.blob(b);
    w->write(&r); std::cout<<"  -> id="<<id<<" blob="<<n<<std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
  }
  return 0;
}
int main(int argc, char** argv){
  int dom = argc>1?atoi(argv[1]):0; std::string e = argc>2?argv[2]:"m"; int n = argc>3?atoi(argv[3]):50;
  auto p = DomainParticipantFactory::get_instance()->create_participant(dom, PARTICIPANT_QOS_DEFAULT);
  if(e=="f") return run<RichF,RichFPubSubType>(p,"RichF",n);
  if(e=="a") return run<RichA,RichAPubSubType>(p,"RichA",n);
  return run<RichM,RichMPubSubType>(p,"RichM",n);
}
