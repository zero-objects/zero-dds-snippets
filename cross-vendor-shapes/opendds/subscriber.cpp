#include "ShapeTypeTypeSupportImpl.h"
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/StaticIncludes.h>
#if OPENDDS_DO_MANUAL_STATIC_INCLUDES
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif
#include <dds/DdsDcpsInfrastructureC.h>
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
  DDS::Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DDS::DataReader_var dr = sub->create_datareader(topic, DATAREADER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  ShapeTypeDataReader_var r = ShapeTypeDataReader::_narrow(dr);
  std::cout << "[opendds-sub] Topic=Square Domain=0" << std::endl;
  for (int i = 0; i < 200; ++i) {
    ShapeType s; DDS::SampleInfo info;
    while (r->take_next_sample(s, info) == DDS::RETCODE_OK) {
      if (info.valid_data)
        std::cout << "  <- color=" << s.color.in() << " x=" << s.x << " y=" << s.y << " size=" << s.shapesize << " RECEIVED" << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  return 0;
}
