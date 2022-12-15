#include "TrafficStorage.hpp"

#include "net/Packet.hpp"
#include "proc/ProcessIndex.hpp"
#include "util/HumanReadable.hpp"

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

namespace ntmd {

TrafficStorage::TrafficStorage(std::filesystem::path dbPath, int interval) :
    mDB(dbPath), mInterval(interval)
{
    this->displayLoop();
}

void TrafficStorage::add(const Process& process, const Packet& pkt)
{
    /* Get existing traffic line for application, or create an empty one */
    TrafficLine& line = mApplicationTraffic[process.comm];

    std::unique_lock<std::mutex> lock(mMutex);
    if (pkt.direction == Direction::Incoming)
    {
        line.bytesRx += pkt.len;
        line.pktRxCount++;
    }
    else
    {
        line.bytesTx += pkt.len;
        line.pktTxCount++;
    }
}

void TrafficStorage::displayLoop()
{
    std::thread loop([this] {
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(mInterval));

            std::unique_lock<std::mutex> lock(mMutex);

            std::cerr << "Application Traffic:\n";
            for (const auto& [name, line] : mApplicationTraffic)
            {
                std::cerr << "  ";
                std::cerr << name
                          << " { rx: " << util::bytesToHumanOvertime(line.bytesRx, mInterval)
                          << ", tx: " << util::bytesToHumanOvertime(line.bytesTx, mInterval)
                          << ", rxc: " << line.pktRxCount << ", txc: " << line.pktTxCount << " }\n";
            }
            std::cerr << "\n";

            mDB.insertApplicationTraffic(mApplicationTraffic);
            mApplicationTraffic.clear();
        }
    });
    loop.detach();
}

} // namespace ntmd