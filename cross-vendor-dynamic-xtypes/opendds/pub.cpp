#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include "dynTypeSupportImpl.h"
#include <ace/OS_NS_unistd.h>
#include <iostream>
int main(int argc, char** argv){
  auto dpf = TheParticipantFactoryWithArgs(argc, argv);
  auto dp = dpf->create_participant(0, PARTICIPANT_QOS_DEFAULT, 0, 0);
  dyn::TrackTypeSupport_var ts = new dyn::TrackTypeSupportImpl;
  ts->register_type(dp, "");
  CORBA::String_var tn = ts->get_type_name();
  auto topic = dp->create_topic("TrackTopic", tn, TOPIC_QOS_DEFAULT, 0, 0);
  auto pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0, 0);
  auto dw = pub->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, 0, 0);
  auto w = dyn::TrackDataWriter::_narrow(dw);
  std::cout << "[od-pub] topic=TrackTopic type=" << tn.in() << std::endl;
  for(CORBA::Long i=1;;i++){
    dyn::Track s;
    s.id = 100+i;
    std::string nm = std::string("otrack-")+std::to_string(i);
    s.name = CORBA::string_dup(nm.c_str());
    s.hue = dyn::BLUE;
    s.path.length(3); s.path[0]=i; s.path[1]=i*2; s.path[2]=i*3;
    int k=i%3;
    if(k==0) s.command.speed(7000+i);
    else if(k==1) s.command.angle(1.5+i);
    else s.command.flag((CORBA::Octet)(i&0xff));
    w->write(s, DDS::HANDLE_NIL);
    std::cout << "  -> id=" << (100+i) << " case=" << k << std::endl;
    ACE_OS::sleep(ACE_Time_Value(0,400000));
  }
}
