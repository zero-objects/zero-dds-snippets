#include "MapEnumPubSubTypes.hpp"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/core/policy/QosPolicies.hpp>
#include <thread>
#include <iostream>
using namespace eprosima::fastdds::dds;
int main(int argc,char**argv){
  int dom=argc>1?atoi(argv[1]):0;
  auto p=DomainParticipantFactory::get_instance()->create_participant(dom,PARTICIPANT_QOS_DEFAULT);
  TypeSupport ts(new feat::MapEnumPubSubType()); ts.register_type(p);
  Topic* tp=p->create_topic("MapEnum",ts.get_type_name(),TOPIC_QOS_DEFAULT);
  auto sub=p->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
  DataReaderQos rq=DATAREADER_QOS_DEFAULT; rq.reliability().kind=RELIABLE_RELIABILITY_QOS;
  rq.representation().m_value.push_back(XCDR_DATA_REPRESENTATION);
  rq.representation().m_value.push_back(XCDR2_DATA_REPRESENTATION);
  auto r=sub->create_datareader(tp,rq);
  std::cout<<"[fd-mapenum-sub] type="<<ts.get_type_name()<<std::endl;
  for(;;){ feat::MapEnum s; SampleInfo info;
    while(r->take_next_sample(&s,&info)==eprosima::fastdds::dds::RETCODE_OK){
      if(!info.valid_data) continue;
      bool ok = s.h()==feat::Hue::H_BLUE && s.m().size()==1 && s.m().at(3).x()==11 && s.m().at(3).y()==12
                && s.sels().size()==1 && s.sels()[0]._d()==2 && s.sels()[0].n()==9;
      std::cout<<"  <- h="<<(int)s.h()<<" m.size="<<s.m().size()<<" sels.size="<<s.sels().size()
               <<" integrity="<<(ok?"OK":"BAD")<<std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
