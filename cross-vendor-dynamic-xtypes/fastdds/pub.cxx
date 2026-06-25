#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include "dynPubSubTypes.hpp"
#include <thread>
#include <chrono>
#include <iostream>
using namespace eprosima::fastdds::dds;
int main(int argc, char** argv){
  int dom = argc>1 ? atoi(argv[1]) : 0;
  auto p = DomainParticipantFactory::get_instance()->create_participant(dom, PARTICIPANT_QOS_DEFAULT);
  TypeSupport ts(new dyn::TrackPubSubType()); ts.register_type(p);
  Topic* tp = p->create_topic("TrackTopic", ts.get_type_name(), TOPIC_QOS_DEFAULT);
  Publisher* pub = p->create_publisher(PUBLISHER_QOS_DEFAULT);
  DataWriter* w = pub->create_datawriter(tp, DATAWRITER_QOS_DEFAULT);
  std::cout << "FastDDS pub: type=" << ts.get_type_name() << " topic=TrackTopic dom=" << dom << std::endl;
  for(int i=0;i<60;i++){
    dyn::Track t;
    t.id(100+i);
    t.name(std::string("track-")+std::to_string(i));
    t.color(dyn::Color::BLUE);
    t.path({i, i*2, i*3});
    dyn::Cmd c;
    int k = i % 3;
    if(k==0) c.speed(7000+i);           // case 1
    else if(k==1) c.angle(1.5+i);       // case 2
    else c.flag((uint8_t)(i&0xff));     // default
    t.command(c);
    w->write(&t);
    std::cout << "  -> id="<<t.id()<<" cmd_case="<<k<<std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  return 0;
}
