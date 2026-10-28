// Fast DDS publisher of the OMG ShapesDemo type, for the cross-vendor proof.
//   shapes_fd_pub <topic> <color> <domain>
// Writes one moving shape every 150 ms until it is stopped.
#include "ShapeTypePubSubTypes.hpp"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
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
  std::string color = argc > 2 ? argv[2] : "RED";
  int dom = argc > 3 ? atoi(argv[3]) : 0;
  auto* p = DomainParticipantFactory::get_instance()->create_participant(dom, PARTICIPANT_QOS_DEFAULT);
  TypeSupport ts(new ShapeTypePubSubType());
  ts.register_type(p);
  Topic* tp = p->create_topic(topic, ts.get_type_name(), TOPIC_QOS_DEFAULT);
  Publisher* pub = p->create_publisher(PUBLISHER_QOS_DEFAULT);
  DataWriterQos wq = DATAWRITER_QOS_DEFAULT;
  wq.reliability().kind = RELIABLE_RELIABILITY_QOS;
  DataWriter* w = pub->create_datawriter(tp, wq);
  std::cout << "[fd-pub] topic=" << topic << " color=" << color << std::endl;
  for (int i = 0;; i++) {
    ShapeType s;
    s.color(color);
    s.x(120 + (i * 11) % 120);
    s.y(225 - (i * 2) % 100);
    s.shapesize(30);
    w->write(&s);
    std::cout << "  -> color=" << s.color() << " x=" << s.x() << " y=" << s.y() << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
  }
}
