#include "RichPubSubTypes.hpp"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/core/policy/QosPolicies.hpp>
#include <thread>
#include <string>
#include <iostream>
using namespace eprosima::fastdds::dds;
template<class T, class TS>
int run(DomainParticipant* p, const char* topic){
  TypeSupport ts(new TS()); ts.register_type(p);
  Topic* tp = p->create_topic(topic, ts.get_type_name(), TOPIC_QOS_DEFAULT);
  Subscriber* sub = p->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
  DataReaderQos rq = DATAREADER_QOS_DEFAULT;
  rq.reliability().kind = RELIABLE_RELIABILITY_QOS;
  rq.representation().m_value.push_back(XCDR_DATA_REPRESENTATION);
  rq.representation().m_value.push_back(XCDR2_DATA_REPRESENTATION);
  DataReader* r = sub->create_datareader(tp, rq);
  std::cout<<"[fd-sub] topic="<<topic<<std::endl;
  for(;;){
    T s; SampleInfo info;
    while(r->take_next_sample(&s,&info)==eprosima::fastdds::dds::RETCODE_OK){
      if(!info.valid_data) continue;
      bool ok=true; auto& b=s.blob(); for(size_t k=0;k<b.size();k++) if(b[k]!=(uint8_t)(k%251)){ok=false;break;}
      std::cout<<"  <- id="<<s.id()<<" value="<<s.value()<<" blob="<<b.size()<<" integrity="<<(ok?"OK":"CORRUPT")<<std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
int main(int argc, char** argv){
  int dom = argc>1?atoi(argv[1]):0; std::string e = argc>2?argv[2]:"m";
  auto p = DomainParticipantFactory::get_instance()->create_participant(dom, PARTICIPANT_QOS_DEFAULT);
  if(e=="f") return run<RichF,RichFPubSubType>(p,"RichF");
  if(e=="a") return run<RichA,RichAPubSubType>(p,"RichA");
  return run<RichM,RichMPubSubType>(p,"RichM");
}
