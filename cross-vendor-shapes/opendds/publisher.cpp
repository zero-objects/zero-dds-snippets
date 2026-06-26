#include "ShapeTypeTypeSupportImpl.h"
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/StaticIncludes.h>
#if OPENDDS_DO_MANUAL_STATIC_INCLUDES
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif
#include <dds/DdsDcpsInfrastructureC.h>
#include <cmath>
#include <iostream>
#include <thread>
#include <chrono>

int ACE_TMAIN(int argc, ACE_TCHAR* argv[]) {
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DDS::DomainParticipant_var dp = dpf->create_participant(0, PARTICIPANT_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  ShapeTypeTypeSupport_var ts = new ShapeTypeTypeSupportImpl;
  ts->register_type(dp, "");
  CORBA::String_var tn = ts->get_type_name();
  DDS::Topic_var topic = dp->create_topic("Square", tn, TOPIC_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DDS::Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DDS::DataWriter_var dw = pub->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  ShapeTypeDataWriter_var w = ShapeTypeDataWriter::_narrow(dw);
  ShapeType s; s.color = "GREEN"; s.shapesize = 30;
  DDS::InstanceHandle_t h = w->register_instance(s);
  std::cout << "[opendds-pub] Topic=Square Domain=0" << std::endl;
  // Wait until at least one subscriber is associated before writing, so the
  // first sample is sequence number 1 with no late-join history gap.
  for (int i = 0; i < 100; ++i) {
    DDS::PublicationMatchedStatus ms; dw->get_publication_matched_status(ms);
    if (ms.current_count >= 1) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  for (double t = 0; t < 1e9; t += 0.15) {
    s.x = (long)(120 + 80 * std::sin(t));
    s.y = (long)(135 + 90 * std::cos(t * 1.3));
    w->write(s, h);
    std::cout << "  -> color=GREEN x=" << s.x << " y=" << s.y << " size=30" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return 0;
}
