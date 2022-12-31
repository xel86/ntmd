#include "DBController.hpp"
#include "util/FilesystemUtil.hpp"
#include "util/StringUtil.hpp"

#include <cstring>
#include <filesystem>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace ntmd {

using TrafficMap = std::unordered_map<std::string, TrafficLine>;

DBController::DBController(std::filesystem::path dbPath)
{
    if (dbPath.empty())
    {
        /* If user is running as root use default db path /var/lib/ntmd.db */
        /* If user is not running as root use default db path ~/.ntmd.db */
        if (geteuid() == 0)
        {
            dbPath = "/var/lib/ntmd.db";
        }
        else
        {
            std::filesystem::path home = util::getHomeDirectory();
            if (home.empty())
            {
                std::cerr
                    << "Could not get location of home directory from $XDG_CONFIG_HOME or $HOME. "
                       "Falling back to root directory config location /var/lib/ntmd.db.\n";
                dbPath = "/var/lib/ntmd.db";
            }
            else
            {
                dbPath = home / ".ntmd.db";
            }
        }
    }

    int error = sqlite3_open(dbPath.c_str(), &mHandle);
    if (error)
    {
        std::cerr << "Error opening or creating database at " << dbPath
                  << " with error: " << sqlite3_errmsg(mHandle) << "\n";
        std::cerr << "Cannot proceed without database connection, exiting.\n";
        exit(1);
    }
    std::cerr << "Successfully opened or created database file at: " << dbPath << "\n";
}

DBController::~DBController() { sqlite3_close(mHandle); }

void DBController::insertApplicationTraffic(const TrafficMap& traffic) const
{
    char* err;
    sqlite3_exec(mHandle, "BEGIN TRANSACTION", nullptr, nullptr, &err);

    const char* sqlCreateTable = "CREATE TABLE IF NOT EXISTS \"%s\" ("
                                 "timestamp INT PRIMARY KEY NOT NULL, "
                                 "bytesRx INT DEFAULT 0, "
                                 "bytesTx INT DEFAULT 0, "
                                 "pktRxCount INT DEFAULT 0, "
                                 "pktTxCount INT DEFAULT 0);";

    const char* sqlInsertValues = "INSERT INTO \"%s\" VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt;
    time_t timestamp = std::time(nullptr);

    // TODO: Can this be done faster/prettier?
    for (const auto& [name, line] : traffic)
    {
        if (util::containsSemicolon(name))
        {
            std::cerr << "Tried inserting traffic into database for an application with a comm "
                         "name that contains non-alphanumeric character(s): "
                      << name
                      << "\n"
                         "Skipping to avoid potential sql injection.\n";
            continue;
        }

        char sqlReplaced[512];
        snprintf(sqlReplaced, 512, sqlCreateTable, name.c_str());

        sqlite3_prepare_v3(mHandle, sqlReplaced, 512, 0, &stmt, nullptr);
        int ret = sqlite3_step(stmt);
        if (ret != SQLITE_DONE)
        {
            std::cerr << "Commit failed while trying to create table.\n";
        }

        sqlite3_reset(stmt);

        snprintf(sqlReplaced, 512, sqlInsertValues, name.c_str());
        sqlite3_prepare_v3(mHandle, sqlReplaced, 512, 0, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, timestamp);
        sqlite3_bind_int(stmt, 2, line.bytesRx);
        sqlite3_bind_int(stmt, 3, line.bytesTx);
        sqlite3_bind_int(stmt, 4, line.pktRxCount);
        sqlite3_bind_int(stmt, 5, line.pktTxCount);

        ret = sqlite3_step(stmt);
        if (ret != SQLITE_DONE)
        {
            std::cerr << "Commit failed while trying to insert application traffic.\n";
        }

        sqlite3_reset(stmt);
    }

    sqlite3_exec(mHandle, "COMMIT TRANSACTION", nullptr, nullptr, &err);
    sqlite3_finalize(stmt);
}

TrafficMap DBController::fetchTrafficSince(time_t timestamp) const
{
    char sql[256];
    snprintf(sql, 256, "select * from \"%%s\" where timestamp >= %ld;", timestamp);

    return fetchTrafficWithQuery(sql);
}

TrafficMap DBController::fetchTrafficBetween(time_t start, time_t end) const
{
    char sql[256];
    snprintf(sql, 256, "select * from \"%%s\" where timestamp >= %ld and timestamp <= %ld;", start,
             end);

    return fetchTrafficWithQuery(sql);
}

std::vector<std::string> DBController::fetchApplicationNames() const
{
    const char* sqlGetApps = "select name from sqlite_schema where type='table';";
    std::vector<std::string> names;

    sqlite3_stmt* stmt;
    sqlite3_prepare_v3(mHandle, sqlGetApps, strlen(sqlGetApps), 0, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

        if (name != nullptr)
            names.push_back(name);
    }

    sqlite3_finalize(stmt);
    return names;
}

TrafficMap DBController::fetchTrafficWithQuery(const char* query) const
{
    auto tables = fetchApplicationNames();
    TrafficMap traffic;

    for (const std::string& name : tables)
    {
        TrafficLine line{};

        char sqlReplaced[256];
        snprintf(sqlReplaced, 256, query, name.c_str());

        sqlite3_stmt* stmt;
        sqlite3_prepare_v3(mHandle, sqlReplaced, 256, 0, &stmt, nullptr);
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            line.bytesRx += sqlite3_column_int64(stmt, 1);
            line.bytesTx += sqlite3_column_int64(stmt, 2);
            line.pktRxCount += sqlite3_column_int64(stmt, 3);
            line.pktTxCount += sqlite3_column_int64(stmt, 4);
        }

        /* If no traffic rows were returned from table query, don't include in traffic map. */
        if (!line.empty())
            traffic[name] = line;

        sqlite3_finalize(stmt);
    }

    return traffic;
}

} // namespace ntmd