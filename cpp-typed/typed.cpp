#include "gen/sensor_msgs/msg/Temperature.hpp"
#include "zerodds/dds.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main() {
    using namespace std::chrono_literals;

    zerodds::Runtime rt(0);
    zerodds::TypedWriter<sensor_msgs::msg::Temperature> w(rt, "Temp");

    // Exact website snippet: the idlc-cpp codegen now emits a field-order
    // constructor, so brace-init works unchanged.
    sensor_msgs::msg::Temperature t{23, "A7"};
    w.write(t);

    // Round-trip check via TypedReader<T> so the demo actually observes a
    // decoded sample, not just a successful publish.
    zerodds::TypedReader<sensor_msgs::msg::Temperature> r(rt, "Temp");
    w.wait_for_matched(1, 5000);
    r.wait_for_matched(1, 5000);
    w.write(t);
    std::this_thread::sleep_for(200ms);
    sensor_msgs::msg::Temperature got;
    if (r.take(got)) {
        std::cout << "got: celsius=" << got.celsius()
                  << " sensor_id=" << got.sensor_id() << "\n";
    } else {
        std::cout << "no sample\n";
    }
    return 0;
}
