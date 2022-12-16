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

class DBController
{
    using TrafficMap = std::unordered_map<std::string, TrafficLine>;

  public:
    /* Opens or creates database at the given path.
     * If the database is being created at the path it will be initalized with the correct schema.*/
    DBController(std::filesystem::path dbPath);
    ~DBController();

    /* Take the built up network traffic from TrafficStorage over the time interval
     * and deposit the traffic into the database by application name and timestamp. */
    void insertApplicationTraffic(const TrafficMap& traffic) const;

    /* Fetch all traffic monitored after the given timestamp. */
    TrafficMap fetchTrafficSince(time_t timestamp) const;

    /* Fetch all traffic monitored between two time periods.
     * start timestamp should be greater than end. */
    TrafficMap fetchTrafficBetween(time_t start, time_t end) const;

  private:
    /* Load application names from database tables into an empty traffic map. */
    void fetchApplicationNames(TrafficMap& traffic) const;

    /* Generic common operation to grab a traffic from every
     * application table based on a query for each individual table.
     * The only format specifier left in the query string should be for the table name (%s). */
    TrafficMap fetchTrafficWithQuery(const char* query) const;

    sqlite3* mHandle{nullptr};
};

} // namespace ntmd
