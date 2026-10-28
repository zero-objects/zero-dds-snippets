// RTI Connext publisher for the reflective-decode example: the `Track` struct
// of ../dyn_app.idl on topic `TrackTopic`, cycling through all three branches
// of the `@appendable` union so a reader that resolves the type at runtime has
// to walk every one of them.
//
// The counterpart of ../fastdds/pub.cxx and ../opendds/pub.cpp, in RTI's modern
// C++11 API. Built by run.sh from the type support `rtiddsgen` emits.
//
// Two things are not the generated example's defaults, and both are needed for
// a foreign reader to see anything:
//
//   * the writer is RELIABLE — the generated example profile is best-effort,
//     and a reliable ZeroDDS reader matches a best-effort writer for discovery
//     only, never for data.
//   * the participant is UDPv4-only — see USER_QOS_PROFILES.xml. Same-host,
//     RTI prefers shared memory, which no foreign stack can read.
#include <dds/dds.hpp>
#include "dyn_app.hpp"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char **argv)
{
    int dom = argc > 1 ? atoi(argv[1]) : 0;
    int count = argc > 2 ? atoi(argv[2]) : 60;

    dds::domain::DomainParticipant p(dom);
    dds::topic::Topic< ::dyn::Track> topic(p, "TrackTopic");

    dds::pub::qos::DataWriterQos wq =
        dds::pub::Publisher(p).default_datawriter_qos();
    wq << dds::core::policy::Reliability::Reliable();
    dds::pub::DataWriter< ::dyn::Track> w(dds::pub::Publisher(p), topic, wq);

    std::cout << "[rti-pub] topic=TrackTopic type=dyn::Track domain=" << dom
              << std::endl;

    // rtiddsgen's C++11 mapping gives an aggregate public members and a union
    // setters that carry the discriminator, so the two halves of the sample
    // are written differently.
    for (int i = 0; i < count; i++) {
        ::dyn::Track t;
        t.id = 100 + i;
        t.name = std::string("rtrack-") + std::to_string(i);
        t.hue = ::dyn::Color::BLUE;
        t.path.resize(3);
        t.path[0] = i;
        t.path[1] = i * 2;
        t.path[2] = i * 3;

        int k = i % 3;
        if (k == 0) {
            t.command.speed(7000 + i);            // case 1
        } else if (k == 1) {
            t.command.angle(1.5 + i);             // case 2
        } else {
            t.command.flag((uint8_t) (i & 0xff)); // default
        }

        w.write(t);
        std::cout << "  -> id=" << t.id << " cmd_case=" << k << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }
    return 0;
}
