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
  auto sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, 0); \
  DDS::DataReaderQos rq; sub->get_default_datareader_qos(rq); rq.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS; \
  auto dr = sub->create_datareader(topic, rq, 0, 0); \
  auto r = T##DataReader::_narrow(dr); \
  std::cout<<"[od-sub] topic="<<TOPIC<<std::endl; \
  for(;;){ T##Seq data; DDS::SampleInfoSeq infos; \
    if(r->take(data, infos, 20, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE)==DDS::RETCODE_OK){ \
      for(CORBA::ULong i=0;i<data.length();i++){ if(!infos[i].valid_data) continue; bool ok=true; \
        for(CORBA::ULong k=0;k<data[i].blob.length();k++) if(data[i].blob[k]!=(k%251)){ok=false;break;} \
        std::cout<<"  <- id="<<data[i].id<<" value="<<data[i].value<<" blob="<<data[i].blob.length()<<" integrity="<<(ok?"OK":"CORRUPT")<<std::endl; } \
      r->return_loan(data, infos); } \
    ACE_OS::sleep(ACE_Time_Value(0,100000)); } }
int ACE_TMAIN(int argc, ACE_TCHAR* argv[]) {
  auto dpf = TheParticipantFactoryWithArgs(argc, argv);
  auto dp = dpf->create_participant(0, PARTICIPANT_QOS_DEFAULT, 0, 0);
  std::string e = argc>1?ACE_TEXT_ALWAYS_CHAR(argv[1]):"m";
  if(e=="f") RUN(RichF,"RichF") else if(e=="a") RUN(RichA,"RichA") else RUN(RichM,"RichM")
  return 0;
}
