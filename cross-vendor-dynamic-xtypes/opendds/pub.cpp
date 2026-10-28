// OpenDDS publisher of dyn::Track: 40 samples, one every 400 ms, cycling
// through the three branches of the union. Usage: od_pub [-DCPS...] [domain]
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/StaticIncludes.h>
#if OPENDDS_DO_MANUAL_STATIC_INCLUDES
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif
#include "dynTypeSupportImpl.h"
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_unistd.h>
#include <iostream>
#include <string>
int ACE_TMAIN(int argc, ACE_TCHAR* argv[]) {
  auto dpf = TheParticipantFactoryWithArgs(argc, argv);
  const int domain = argc > 1 ? ACE_OS::atoi(argv[1]) : 0;
  auto dp = dpf->create_participant(domain, PARTICIPANT_QOS_DEFAULT, 0, 0);
  dyn::TrackTypeSupport_var ts = new dyn::TrackTypeSupportImpl;
  ts->register_type(dp, "");
  CORBA::String_var tn = ts->get_type_name();
  auto topic = dp->create_topic("TrackTopic", tn, TOPIC_QOS_DEFAULT, 0, 0);
  auto pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0, 0);
  auto dw = pub->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, 0, 0);
  auto w = dyn::TrackDataWriter::_narrow(dw);
  std::cout << "[od-pub] topic=TrackTopic type=" << tn.in() << " domain=" << domain << std::endl;
  for (CORBA::Long i = 1; i <= 40; i++) {
    dyn::Track s;
    s.id = 100 + i;
    const std::string nm = std::string("otrack-") + std::to_string(i);
    s.name = CORBA::string_dup(nm.c_str());
    s.hue = dyn::BLUE;
    s.path.length(3); s.path[0] = i; s.path[1] = i * 2; s.path[2] = i * 3;
    const int k = i % 3;
    if (k == 0) s.command.speed(7000 + i);
    else if (k == 1) s.command.angle(1.5 + i);
    else s.command.flag(static_cast<CORBA::Octet>(i & 0xff));
    const DDS::ReturnCode_t rc = w->write(s, DDS::HANDLE_NIL);
    std::cout << "  -> id=" << (100 + i) << " case=" << k << " rc=" << rc << std::endl;
    ACE_OS::sleep(ACE_Time_Value(0, 400000));
  }
  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();
  return 0;
}
