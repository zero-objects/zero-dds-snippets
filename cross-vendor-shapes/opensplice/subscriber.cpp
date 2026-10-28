// OpenSplice 6.9 (classic C++ API, SACPP) subscriber of ShapeType on `Square`.
// Prints one `<-` line per valid sample. The domain is the one of $OSPL_URI.
#include <chrono>
#include <iostream>
#include <thread>
#include "ccpp_dds_dcps.h"
#include "ccpp_ShapeType.h"

int main() {
  DDS::DomainParticipantFactory_var dpf = DDS::DomainParticipantFactory::get_instance();
  DDS::DomainParticipant_var dp = dpf->create_participant(
      DDS::DOMAIN_ID_DEFAULT, PARTICIPANT_QOS_DEFAULT, NULL, DDS::STATUS_MASK_NONE);
  if (!dp.in()) { std::cerr << "[opensplice-sub] no participant (OSPL_URI?)" << std::endl; return 1; }
  ShapeTypeTypeSupport_var ts = new ShapeTypeTypeSupport;
  ts->register_type(dp.in(), "ShapeType");
  DDS::Topic_var topic = dp->create_topic("Square", "ShapeType", TOPIC_QOS_DEFAULT, NULL, DDS::STATUS_MASK_NONE);
  DDS::Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, NULL, DDS::STATUS_MASK_NONE);
  DDS::DataReader_var dr = sub->create_datareader(topic.in(), DATAREADER_QOS_DEFAULT, NULL, DDS::STATUS_MASK_NONE);
  ShapeTypeDataReader_var r = ShapeTypeDataReader::_narrow(dr.in());
  std::cout << "[opensplice-sub] Topic=Square" << std::endl;
  for (;;) {
    ShapeTypeSeq samples;
    DDS::SampleInfoSeq infos;
    if (r->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE,
                DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE) == DDS::RETCODE_OK) {
      for (DDS::ULong i = 0; i < samples.length(); ++i) {
        if (!infos[i].valid_data) continue;
        std::cout << "  <- color=" << samples[i].color.in() << " x=" << samples[i].x
                  << " y=" << samples[i].y << " size=" << samples[i].shapesize << std::endl;
      }
      r->return_loan(samples, infos);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  return 0;
}
