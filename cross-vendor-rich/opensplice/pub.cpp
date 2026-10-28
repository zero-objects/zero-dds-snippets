// OpenSplice 6.9 (classic C++ API, SACPP) publisher of RichF on `RichF`:
// id = 1, 2, ...; value = pi * id; blob[k] = k % 251. The domain is the
// one of $OSPL_URI, which run.sh writes.
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include "ccpp_dds_dcps.h"
#include "ccpp_Rich.h"

int main(int argc, char** argv) {
  const int n = argc > 1 ? std::atoi(argv[1]) : 50;
  DDS::DomainParticipantFactory_var dpf = DDS::DomainParticipantFactory::get_instance();
  DDS::DomainParticipant_var dp = dpf->create_participant(
      DDS::DOMAIN_ID_DEFAULT, PARTICIPANT_QOS_DEFAULT, NULL, DDS::STATUS_MASK_NONE);
  if (!dp.in()) { std::cerr << "[ospl-pub] no participant (OSPL_URI?)" << std::endl; return 1; }
  RichFTypeSupport_var ts = new RichFTypeSupport;
  ts->register_type(dp.in(), "RichF");
  DDS::Topic_var topic = dp->create_topic("RichF", "RichF", TOPIC_QOS_DEFAULT, NULL, DDS::STATUS_MASK_NONE);
  DDS::Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, NULL, DDS::STATUS_MASK_NONE);
  DDS::DataWriter_var dw = pub->create_datawriter(topic.in(), DATAWRITER_QOS_DEFAULT, NULL, DDS::STATUS_MASK_NONE);
  RichFDataWriter_var w = RichFDataWriter::_narrow(dw.in());
  // Wait until a subscriber is matched before writing.
  for (int i = 0; i < 100; ++i) {
    DDS::PublicationMatchedStatus ms;
    dw->get_publication_matched_status(ms);
    if (ms.current_count >= 1) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::cout << "[ospl-pub] topic=RichF blob=" << n << std::endl;
  RichF s;
  for (DDS::ULong id = 1;; ++id) {
    s.id = id;
    s.value = 3.141592653589793 * id;
    s.blob.length(n);
    for (int k = 0; k < n; ++k) s.blob[k] = static_cast<DDS::Octet>(k % 251);
    w->write(s, DDS::HANDLE_NIL);
    std::cout << "  -> id=" << id << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
  }
  return 0;
}
