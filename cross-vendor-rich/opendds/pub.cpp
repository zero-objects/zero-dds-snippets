#include "RichTypeSupportImpl.h"
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/StaticIncludes.h>
#if OPENDDS_DO_MANUAL_STATIC_INCLUDES
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif
#include <iostream>
#include <string>
#include <thread>
#define RUN(T, TOPIC) { \
  T##TypeSupport_var ts = new T##TypeSupportImpl; ts->register_type(dp, ""); \
  CORBA::String_var tn = ts->get_type_name(); \
  auto topic = dp->create_topic(TOPIC, tn, TOPIC_QOS_DEFAULT, 0, 0); \
  auto pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0, 0); \
  auto dw = pub->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, 0, 0); \
  auto w = T##DataWriter::_narrow(dw); \
  for(int i=0;i<100;i++){ DDS::PublicationMatchedStatus ms; dw->get_publication_matched_status(ms); if(ms.current_count>=1)break; ACE_OS::sleep(ACE_Time_Value(0,100000)); } \
  std::cout<<"[od-pub] topic="<<TOPIC<<" blob="<<n<<std::endl; \
  T s; for(CORBA::ULong id=1;;id++){ s.id=id; s.value=3.141592653589793*id; s.blob.length(n); for(int k=0;k<n;k++) s.blob[k]=k%251; w->write(s, DDS::HANDLE_NIL); std::cout<<"  -> id="<<id<<std::endl; ACE_OS::sleep(ACE_Time_Value(0,150000)); } }
int ACE_TMAIN(int argc, ACE_TCHAR* argv[]) {
  auto dpf = TheParticipantFactoryWithArgs(argc, argv);
  auto dp = dpf->create_participant(0, PARTICIPANT_QOS_DEFAULT, 0, 0);
  std::string e = argc>1?ACE_TEXT_ALWAYS_CHAR(argv[1]):"m";
  int n = argc>2?ACE_OS::atoi(argv[2]):50;
  if(e=="f") RUN(RichF,"RichF") else if(e=="a") RUN(RichA,"RichA") else RUN(RichM,"RichM")
  return 0;
}
