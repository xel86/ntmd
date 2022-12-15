#pragma once

#include <filesystem>
#include <sqlite3.h>
#include <string>
#include <unordered_map>

namespace ntmd {

struct TrafficLine
{
    uint64_t bytesRx{0}; /* Bytes received. */
    uint64_t bytesTx{0}; /* Bytes transmitted. */
    int pktRxCount{0};   /* Number of packets received. */
    int pktTxCount{0};   /* Number of packets transmitted. */
};

class DBConnector
{
  public:
    /* Opens or creates database at the given path.
     * If the database is being created at the path it will be initalized with the correct schema.*/
    DBConnector(std::filesystem::path dbPath);
    ~DBConnector();

    /* Take the built up network traffic from TrafficStorage over the time interval
     * and deposit the traffic into the database by application name and timestamp. */
    void insertApplicationTraffic(const std::unordered_map<std::string, TrafficLine>& traffic);

  private:
    sqlite3* mHandle{nullptr};
};

} // namespace ntmd
