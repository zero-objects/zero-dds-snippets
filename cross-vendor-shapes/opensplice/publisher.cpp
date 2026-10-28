// OpenSplice 6.9 (classic C++ API, SACPP) publisher of ShapeType on `Square`.
// The domain is the one of $OSPL_URI, which run.sh writes.
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include "ccpp_dds_dcps.h"
#include "ccpp_ShapeType.h"

int main() {
  DDS::DomainParticipantFactory_var dpf = DDS::DomainParticipantFactory::get_instance();
  DDS::DomainParticipant_var dp = dpf->create_participant(
      DDS::DOMAIN_ID_DEFAULT, PARTICIPANT_QOS_DEFAULT, NULL, DDS::STATUS_MASK_NONE);
  if (!dp.in()) { std::cerr << "[opensplice-pub] no participant (OSPL_URI?)" << std::endl; return 1; }
  ShapeTypeTypeSupport_var ts = new ShapeTypeTypeSupport;
  ts->register_type(dp.in(), "ShapeType");
  DDS::Topic_var topic = dp->create_topic("Square", "ShapeType", TOPIC_QOS_DEFAULT, NULL, DDS::STATUS_MASK_NONE);
  DDS::Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, NULL, DDS::STATUS_MASK_NONE);
  DDS::DataWriter_var dw = pub->create_datawriter(topic.in(), DATAWRITER_QOS_DEFAULT, NULL, DDS::STATUS_MASK_NONE);
  ShapeTypeDataWriter_var w = ShapeTypeDataWriter::_narrow(dw.in());
  ShapeType s;
  s.color = DDS::string_dup("GREEN");
  s.shapesize = 30;
  std::cout << "[opensplice-pub] Topic=Square" << std::endl;
  // Wait until a subscriber is matched before writing: a sample written
  // earlier goes to nobody.
  for (int i = 0; i < 100; ++i) {
    DDS::PublicationMatchedStatus ms;
    dw->get_publication_matched_status(ms);
    if (ms.current_count >= 1) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  for (double t = 0; t < 1e9; t += 0.15) {
    s.x = static_cast<DDS::Long>(120 + 80 * std::sin(t));
    s.y = static_cast<DDS::Long>(135 + 90 * std::cos(t * 1.3));
    w->write(s, DDS::HANDLE_NIL);
    std::cout << "  -> color=GREEN x=" << s.x << " y=" << s.y << " size=30" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return 0;
}
