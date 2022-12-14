#include "TrafficStorage.hpp"

#include "Daemon.hpp"
#include "net/Packet.hpp"
#include "proc/ProcessIndex.hpp"
#include "util/HumanReadable.hpp"

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

namespace ntmd {

using TrafficMap = std::unordered_map<std::string, TrafficLine>;

TrafficStorage::TrafficStorage(int interval, const DBController& db) : mDB(db), mInterval(interval)
{
    this->depositLoop();
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

std::pair<TrafficMap, int> TrafficStorage::getLiveSnapshot() const
{
    std::unique_lock<std::mutex> lock(mMutex);
    return {mApplicationTraffic, mInterval};
}

bool TrafficStorage::awaitSnapshot(std::mutex& mutex, TrafficMap& traffic, int& interval)
{
    std::unique_lock<std::mutex> lock(mMutex);

    /* TODO: Only one thread can be waiting (for now). */
    if (mAPIWaiting)
        return false;

    mAPIWaiting = true;
    apiMutex = &mutex;
    apiTrafficMap = &traffic;
    apiInterval = &interval;

    /* Once we have pointers to the desired variables to place the live updated traffic data, lock
     * the mutex so that the caller of this method can wait until we have set them. */
    apiMutex->lock();
    return true;
}

void TrafficStorage::depositLoop()
{
    std::thread loop([this] {
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(mInterval));

            std::unique_lock<std::mutex> lock(mMutex);

            mDB.insertApplicationTraffic(mApplicationTraffic);

            // TODO: multiple listeners?
            /* If the APIController is hooked into the traffic storage and waiting to receive live
             * traffic updates, set the api member variables with our internal traffic structures
             * and let the API controller know it has received the updated traffic by unlocking
             * their mutex.
             */
            if (mAPIWaiting)
            {
                if (apiMutex == nullptr || apiTrafficMap == nullptr || apiInterval == nullptr)
                {
                    std::cerr << ntmd::logdebug
                              << "Live API hook variables were not set despite mAPIWaiting being "
                                 "set true.\n";
                    mAPIWaiting = false;
                }
                else
                {
                    /* These pointers become invalidated the moment the apiMutex is unlocked. */
                    *apiTrafficMap = mApplicationTraffic;
                    *apiInterval = mInterval;
                    mAPIWaiting = false;
                    apiMutex->unlock();
                }
            }

            mApplicationTraffic.clear();
        }
    });
    loop.detach();
}

} // namespace ntmd