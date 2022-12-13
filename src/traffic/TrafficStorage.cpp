#include "TrafficStorage.hpp"

#include "net/Packet.hpp"
#include "proc/ProcessIndex.hpp"
#include "util/HumanReadable.hpp"

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

namespace ntmd {

void TrafficStorage::add(const Process& process, const Packet& pkt)
{
    /* Get existing traffic line for application, or create an empty one */
    TrafficLine& line = mApplicationTraffic[process.comm];

    std::unique_lock<std::mutex> lock(mMutex);
    if (pkt.direction == Direction::Incoming)
    {
        line.pktRx += pkt.len;
        line.pktRxCount++;
    }
    else
    {
        line.pktTx += pkt.len;
        line.pktTxCount++;
    }
}

void TrafficStorage::displayLoop()
{
    std::thread loop([&] {
        const int interval = 5;
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(interval));

            std::unique_lock<std::mutex> lock(mMutex);

            std::cerr << "Application Traffic:\n";
            for (const auto& [name, line] : mApplicationTraffic)
            {
                std::cerr << "  ";
                std::cerr << name << " { rx: " << util::bytesToHumanOvertime(line.pktRx, interval)
                          << ", tx: " << util::bytesToHumanOvertime(line.pktTx, interval)
                          << ", rxc: " << line.pktRxCount << ", txc: " << line.pktTxCount << " }\n";
            }
            std::cerr << "\n";

            mApplicationTraffic.clear();
        }
    });
    loop.detach();
}

} // namespace ntmd