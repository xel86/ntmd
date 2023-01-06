#pragma once

#include "DBController.hpp"
#include "net/Packet.hpp"
#include "proc/ProcessIndex.hpp"

#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace ntmd {

/* In memory storage for network traffic monitored from all processes
 * since the last database deposit. The TrafficStorage is cleared on a
 * set interval after its data is sent to the database. */
class TrafficStorage
{
    using TrafficMap = std::unordered_map<std::string, TrafficLine>;

  public:
    TrafficStorage(int interval, const DBController& db);
    ~TrafficStorage() = default;

    /* Adds the packet length (amount of bytes rx/tx) to
     * the application's total traffic during this interval. */
    void add(const Process& process, const Packet& pkt);

    /* Returns snapshot of whatever traffic data is stored in memory before database deposit.
     * Could be empty if called right after database deposit interval,
     * use awaitSnapshot if this is a concern. */
    std::pair<TrafficMap, int> getLiveSnapshot() const;

    /* Method to synchronously set the live in-memory traffic to the given variables in a different
     * thread. The given mutex must be unlocked, and the caller should immediately try to lock it
     * after this method call. Once the caller has relocked their mutex the traffic and interval
     * variables will be set. */
    bool awaitSnapshot(std::mutex& mutex, TrafficMap& traffic, int& interval);

  private:
    /* Display all applications and their accumulated traffic to stderr.
     * Primarily for debugging. */
    void depositLoop();

    /* Map that stores the total traffic monitored for each application.
     * The string key is the name of the application gathered from
     * the process' comm name */
    TrafficMap mApplicationTraffic{};
    mutable std::mutex mMutex;

    const DBController& mDB;
    int mInterval;

    /* Live API Watchers */
    /* We do not own these variables, they are borrowed from the API Controller
     * that is watching this traffic storage for live in-memory traffic updates. */
    bool mAPIWaiting{false};
    std::mutex* apiMutex;
    TrafficMap* apiTrafficMap;
    int* apiInterval;
};

} // namespace ntmd