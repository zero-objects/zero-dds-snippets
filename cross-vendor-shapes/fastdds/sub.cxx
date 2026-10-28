// Fast DDS subscriber of the OMG ShapesDemo type, for the cross-vendor proof.
//   shapes_fd_sub <topic> <domain>
// Prints one "<-" line per sample until it is stopped.
//
// The reader accepts both data representations. An empty
// DataRepresentationQosPolicy on a reader means XCDR1 only (XTypes 1.3,
// DATA_REPRESENTATION), and ZeroDDS's @final writer sends XCDR2, which a
// stock Fast DDS reader refuses with INCOMPATIBLE_QOS.
#include "ShapeTypePubSubTypes.hpp"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/core/policy/QosPolicies.hpp>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
using namespace eprosima::fastdds::dds;
int main(int argc, char** argv) {
  std::string topic = argc > 1 ? argv[1] : "Square";
  int dom = argc > 2 ? atoi(argv[2]) : 0;
  auto* p = DomainParticipantFactory::get_instance()->create_participant(dom, PARTICIPANT_QOS_DEFAULT);
  TypeSupport ts(new ShapeTypePubSubType());
  ts.register_type(p);
  Topic* tp = p->create_topic(topic, ts.get_type_name(), TOPIC_QOS_DEFAULT);
  Subscriber* sub = p->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
  DataReaderQos rq = DATAREADER_QOS_DEFAULT;
  rq.reliability().kind = RELIABLE_RELIABILITY_QOS;
  rq.representation().m_value.push_back(XCDR_DATA_REPRESENTATION);
  rq.representation().m_value.push_back(XCDR2_DATA_REPRESENTATION);
  DataReader* r = sub->create_datareader(tp, rq);
  std::cout << "[fd-sub] topic=" << topic << std::endl;
  for (;;) {
    ShapeType s;
    SampleInfo info;
    while (r->take_next_sample(&s, &info) == RETCODE_OK) {
      if (!info.valid_data) continue;
      std::cout << "  <- color=" << s.color() << " x=" << s.x() << " y=" << s.y()
                << " size=" << s.shapesize() << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
