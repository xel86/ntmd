#pragma once

#include "net/Packet.hpp"
#include "proc/ProcessIndex.hpp"

#include <mutex>
#include <string>
#include <unordered_map>

namespace ntmd {

struct TrafficLine
{
    uint64_t pktRx{0}; /* Bytes received. */
    uint64_t pktTx{0}; /* Bytes transmitted. */
    int pktRxCount{0}; /* Number of packets received. */
    int pktTxCount{0}; /* Number of packets transmitted. */
};

/* In memory storage for network traffic monitored from all processes
 * since the last database deposit. The TrafficStorage is cleared on a
 * set interval after its data is sent to the database. */
class TrafficStorage
{
  public:
    TrafficStorage() = default;
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
};

} // namespace ntmd