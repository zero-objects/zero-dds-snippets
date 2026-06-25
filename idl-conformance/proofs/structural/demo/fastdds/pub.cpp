#include "MapEnumPubSubTypes.hpp"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
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
  auto pub=p->create_publisher(PUBLISHER_QOS_DEFAULT);
  DataWriterQos wq=DATAWRITER_QOS_DEFAULT; wq.reliability().kind=RELIABLE_RELIABILITY_QOS;
  auto w=pub->create_datawriter(tp,wq);
  for(int i=0;i<100;i++){PublicationMatchedStatus s;w->get_publication_matched_status(s);if(s.current_count>=1)break;std::this_thread::sleep_for(std::chrono::milliseconds(100));}
  feat::MapEnum s; s.h(feat::Hue::H_BLUE);
  feat::Pt pt; pt.x(11); pt.y(12); s.m()[3]=pt;
  feat::Sel sel; sel.n(9); s.sels().push_back(sel);
  std::cout<<"[fd-mapenum-pub]"<<std::endl;
  for(;;){ w->write(&s); std::this_thread::sleep_for(std::chrono::milliseconds(150)); }
}
