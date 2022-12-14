#pragma once

#include "DBConnector.hpp"
#include "net/Packet.hpp"
#include "proc/ProcessIndex.hpp"

#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>

namespace ntmd {

/* In memory storage for network traffic monitored from all processes
 * since the last database deposit. The TrafficStorage is cleared on a
 * set interval after its data is sent to the database. */
class TrafficStorage
{
  public:
    TrafficStorage(std::filesystem::path dbPath, int interval) : mDB(dbPath), mInterval(interval){};
    ~TrafficStorage() = default;

    /* Adds the packet length (amount of bytes rx/tx) to
     * the application's total traffic during this interval. */
    void add(const Process& process, const Packet& pkt);

    /* Display all applications and their accumulated traffic to stderr.
     * Primarily for debugging. */
    void displayLoop();

  private:
    /* Map that stores the total traffic monitored for each application.
     * The string key is the name of the application gathered from
     * the process' comm name */
    std::unordered_map<std::string, TrafficLine> mApplicationTraffic{};
    std::mutex mMutex;

    DBConnector mDB;
    int mInterval;
};

} // namespace ntmd