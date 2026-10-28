// OpenSplice 6.9 (classic C++ API, SACPP) subscriber of RichF on `RichF`,
// best effort, as the other legs' readers. Prints one line per valid sample
// with `integrity=OK` when blob[k] = k % 251 and value = pi * id.
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include "ccpp_dds_dcps.h"
#include "ccpp_Rich.h"

int main() {
  DDS::DomainParticipantFactory_var dpf = DDS::DomainParticipantFactory::get_instance();
  DDS::DomainParticipant_var dp = dpf->create_participant(
      DDS::DOMAIN_ID_DEFAULT, PARTICIPANT_QOS_DEFAULT, NULL, DDS::STATUS_MASK_NONE);
  if (!dp.in()) { std::cerr << "[ospl-sub] no participant (OSPL_URI?)" << std::endl; return 1; }
  RichFTypeSupport_var ts = new RichFTypeSupport;
  ts->register_type(dp.in(), "RichF");
  DDS::Topic_var topic = dp->create_topic("RichF", "RichF", TOPIC_QOS_DEFAULT, NULL, DDS::STATUS_MASK_NONE);
  DDS::Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, NULL, DDS::STATUS_MASK_NONE);
  DDS::DataReaderQos rq;
  sub->get_default_datareader_qos(rq);
  rq.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
  DDS::DataReader_var dr = sub->create_datareader(topic.in(), rq, NULL, DDS::STATUS_MASK_NONE);
  RichFDataReader_var r = RichFDataReader::_narrow(dr.in());
  std::cout << "[ospl-sub] topic=RichF" << std::endl;
  for (;;) {
    RichFSeq data;
    DDS::SampleInfoSeq infos;
    if (r->take(data, infos, 20, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE,
                DDS::ANY_INSTANCE_STATE) == DDS::RETCODE_OK) {
      for (DDS::ULong i = 0; i < data.length(); ++i) {
        if (!infos[i].valid_data) continue;
        bool ok = std::fabs(data[i].value - 3.141592653589793 * data[i].id) < 1e-9;
        for (DDS::ULong k = 0; ok && k < data[i].blob.length(); ++k)
          if (data[i].blob[k] != (k % 251)) ok = false;
        std::cout << "  <- id=" << data[i].id << " value=" << data[i].value
                  << " blob=" << data[i].blob.length() << " integrity=" << (ok ? "OK" : "CORRUPT")
                  << std::endl;
      }
      r->return_loan(data, infos);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return 0;
}
