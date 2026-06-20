#include "zerodds/dds.hpp"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    using namespace std::chrono_literals;

    zerodds::Runtime rt(0);
    zerodds::Writer w(rt, "Chatter", "RawBytes", /*reliable=*/true);
    zerodds::Reader r(rt, "Chatter", "RawBytes", /*reliable=*/true);

    w.wait_for_matched(1, 5000);
    r.wait_for_matched(1, 5000);

    std::vector<uint8_t> payload{'h','e','l','l','o'};
    w.write(payload);

    std::this_thread::sleep_for(200ms);
    auto sample = r.take();
    if (!sample.empty()) {
        std::cout << "got: " << std::string(sample.begin(), sample.end()) << "\n";
    }
    return 0;
}
