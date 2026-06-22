// SPDX-License-Identifier: Apache-2.0
// Minimal Fast-DDS HelloWorld pub/sub used purely to drive comparable RTPS
// wire traffic (SPDP / SEDP / DATA / HEARTBEAT / ACKNACK) for the RTPS-framing
// vendor-oracle. Keyed type so a KeyHash is emitted in inline QoS.
//
// Targets Fast-DDS 3.6 (TopicDataType uses SerializedPayload_t).
//
// Build (codepit):
//   g++ -std=c++17 fastdds_hello.cpp -o fastdds_hello \
//       -I/opt/fastdds/include -L/opt/fastdds/lib \
//       -lfastdds -lfastcdr -Wl,-rpath,/opt/fastdds/lib
//
// Run:
//   ./fastdds_hello pub   # publisher (keyed, reliable, transient_local)
//   ./fastdds_hello sub   # subscriber
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/topic/TopicDataType.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/rtps/common/SerializedPayload.hpp>
#include <fastdds/rtps/common/InstanceHandle.hpp>

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

using namespace eprosima::fastdds::dds;
using eprosima::fastdds::rtps::SerializedPayload_t;

// ---- A tiny keyed type: { @key uint32 id; char[32] msg; } ----
struct Hello {
    uint32_t id;
    char msg[32];
};

// XCDR1 little-endian encapsulation header: 00 01 00 00
class HelloType : public TopicDataType {
public:
    HelloType() {
        set_name("Hello");
        max_serialized_type_size = 4 /*encaps*/ + 4 /*id*/ + 32 /*msg*/ + 4;
        is_compute_key_provided = true;  // keyed
    }
    bool serialize(const void* const data, SerializedPayload_t& payload,
                   DataRepresentationId_t) override {
        const Hello* h = static_cast<const Hello*>(data);
        uint8_t* p = payload.data;
        p[0] = 0x00; p[1] = 0x01; p[2] = 0x00; p[3] = 0x00;  // CDR_LE
        std::memcpy(p + 4, &h->id, 4);          // host LE
        std::memcpy(p + 8, h->msg, 32);
        payload.length = 4 + 4 + 32;
        return true;
    }
    bool deserialize(SerializedPayload_t& payload, void* data) override {
        Hello* h = static_cast<Hello*>(data);
        const uint8_t* p = payload.data;
        std::memcpy(&h->id, p + 4, 4);
        std::memcpy(h->msg, p + 8, 32);
        return true;
    }
    uint32_t calculate_serialized_size(const void* const, DataRepresentationId_t) override {
        return 4 + 4 + 32;
    }
    bool compute_key(SerializedPayload_t& payload, eprosima::fastdds::rtps::InstanceHandle_t& h,
                     bool) override {
        // Key = id (the first member). 16-byte big-endian key hash.
        uint32_t id; std::memcpy(&id, payload.data + 4, 4);
        std::memset(h.value, 0, 16);
        h.value[0] = (id >> 24) & 0xFF; h.value[1] = (id >> 16) & 0xFF;
        h.value[2] = (id >> 8) & 0xFF;  h.value[3] = id & 0xFF;
        return true;
    }
    bool compute_key(const void* const data, eprosima::fastdds::rtps::InstanceHandle_t& h,
                     bool) override {
        const Hello* hh = static_cast<const Hello*>(data);
        std::memset(h.value, 0, 16);
        h.value[0] = (hh->id >> 24) & 0xFF; h.value[1] = (hh->id >> 16) & 0xFF;
        h.value[2] = (hh->id >> 8) & 0xFF;  h.value[3] = hh->id & 0xFF;
        return true;
    }
    void* create_data() override { return new Hello(); }
    void delete_data(void* d) override { delete static_cast<Hello*>(d); }
};

class Listener : public DataReaderListener {
public:
    void on_data_available(DataReader* r) override {
        Hello h; SampleInfo info;
        while (r->take_next_sample(&h, &info) == eprosima::fastdds::dds::RETCODE_OK) {
            if (info.valid_data) std::cout << "sub got id=" << h.id << " msg=" << h.msg << "\n";
        }
    }
};

int main(int argc, char** argv) {
    bool is_pub = (argc > 1 && std::string(argv[1]) == "pub");
    auto* dpf = DomainParticipantFactory::get_instance();
    DomainParticipant* part = dpf->create_participant(0, PARTICIPANT_QOS_DEFAULT);
    TypeSupport ts(new HelloType());
    ts.register_type(part);
    Topic* topic = part->create_topic("HelloTopic", "Hello", TOPIC_QOS_DEFAULT);

    if (is_pub) {
        Publisher* pub = part->create_publisher(PUBLISHER_QOS_DEFAULT);
        DataWriterQos wqos;
        wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        wqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        wqos.history().kind = KEEP_LAST_HISTORY_QOS;
        wqos.history().depth = 10;
        DataWriter* w = pub->create_datawriter(topic, wqos);
        for (int i = 0; i < 30; ++i) {
            Hello h; h.id = 7; std::snprintf(h.msg, 32, "hello #%d", i);
            w->write(&h);
            std::cout << "pub wrote id=7 #" << i << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    } else {
        Subscriber* sub = part->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
        DataReaderQos rqos;
        rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        static Listener l;
        sub->create_datareader(topic, rqos, &l);
        std::this_thread::sleep_for(std::chrono::seconds(20));
    }
    dpf->delete_participant(part);
    return 0;
}
