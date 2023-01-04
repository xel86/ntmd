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
    int execErr;
    execErr = sqlite3_exec(mHandle, "BEGIN TRANSACTION", nullptr, nullptr, &err);
    if (execErr != SQLITE_OK)
    {
        std::cerr << "Error beginning application traffic transaction.\n";
        sqlite3_free(err);
        return;
    }

    /* The best db design that I could think of for performance, space, and simplicity was to
     * have a table per application; unfortunately since the sqlite3 api doesn't allow for
     * binding table names, we must do it dynamically. */
    const char* sqlCreateTable = "CREATE TABLE IF NOT EXISTS \"%s\" ("
                                 "timestamp INT PRIMARY KEY NOT NULL, "
                                 "bytesRx INT DEFAULT 0, "
                                 "bytesTx INT DEFAULT 0, "
                                 "pktRxCount INT DEFAULT 0, "
                                 "pktTxCount INT DEFAULT 0);";

    const char* sqlInsertValues = "INSERT INTO \"%s\" VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt* createTableStmt;
    sqlite3_stmt* insertTrafficStmt;
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

        /* Create application table if it doesn't already exist. */
        char sqlReplaced[512];
        snprintf(sqlReplaced, 512, sqlCreateTable, name.c_str());

        sqlite3_prepare_v2(mHandle, sqlReplaced, -1, &createTableStmt, nullptr);
        int ret = sqlite3_step(createTableStmt);
        if (ret != SQLITE_DONE)
        {
            std::cerr << "Commit failed while trying to create table.\n";
        }

        sqlite3_finalize(createTableStmt);

        /* Deposit traffic line into application table */
        snprintf(sqlReplaced, 512, sqlInsertValues, name.c_str());
        sqlite3_prepare_v2(mHandle, sqlReplaced, -1, &insertTrafficStmt, nullptr);
        sqlite3_bind_int(insertTrafficStmt, 1, timestamp);
        sqlite3_bind_int(insertTrafficStmt, 2, line.bytesRx);
        sqlite3_bind_int(insertTrafficStmt, 3, line.bytesTx);
        sqlite3_bind_int(insertTrafficStmt, 4, line.pktRxCount);
        sqlite3_bind_int(insertTrafficStmt, 5, line.pktTxCount);

        ret = sqlite3_step(insertTrafficStmt);
        if (ret != SQLITE_DONE)
        {
            std::cerr << "Commit failed while trying to insert application traffic.\n";
        }

        sqlite3_finalize(insertTrafficStmt);
    }

    sqlite3_exec(mHandle, "COMMIT TRANSACTION", nullptr, nullptr, &err);
    if (execErr != SQLITE_OK)
    {
        std::cerr << "Error commiting application traffic transaction.\n";
        sqlite3_free(err);
    }

    // TODO: further research into this?
    /* Occasionally sqlite with allocate more memory that persists after these insert statements.
     * When a new table is made or accessed for the first time during runtime the sqlite object will
     * allocate metadata for it to increase performance. Although this isn't that big of an issue,
     * every once and a while it will allocate 4K-9K bytes seemingly randomly despite doing nothing
     * unique from other calls to this function. At first I thought this was new cache pages but
     * upon testing none were being created/used. Overtime this will build up memory usage.
     * Furthermore, when calling functions like fetchTrafficWithQuery the memory used by sqlite will
     * increase by almost 700K bytes and never get freed until I suppose it hits a hard limit
     * despite there being no actual memory leak reported by valgrind. Manually releasing all memory
     * like this is bound to decrease insertion performance but I think it is better than the
     * alternative situation of ever increasing memory. Ideally we could find the proper settings
     * for sqlite to avoid allocating the extra memory in the first place. */
    sqlite3_db_release_memory(mHandle);
}

TrafficMap DBController::fetchTrafficSince(time_t timestamp) const
{
    char sql[256];
    snprintf(sql, 256,
             "select SUM(bytesRx), SUM(bytesTx), SUM(pktRxCount), SUM(pktTxCount) from \"%%s\" "
             "where timestamp >= %ld;",
             timestamp);

    return fetchTrafficWithQuery(sql);
}

TrafficMap DBController::fetchTrafficBetween(time_t start, time_t end) const
{
    char sql[256];
    snprintf(sql, 256,
             "select SUM(bytesRx), SUM(bytesTx), SUM(pktRxCount), SUM(pktTxCount) from \"%%s\" "
             "where timestamp >= %ld and timestamp <= %ld;",
             start, end);

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
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            line.bytesRx = sqlite3_column_int64(stmt, 0);
            line.bytesTx = sqlite3_column_int64(stmt, 1);
            line.pktRxCount = sqlite3_column_int64(stmt, 2);
            line.pktTxCount = sqlite3_column_int64(stmt, 3);
        }
        else
        {
            std::cerr << "Error fetching traffic with query: " << query << "\n";
        }

        /* If no traffic rows were returned from table query, don't include in traffic map. */
        if (!line.empty())
            traffic[name] = line;

        sqlite3_finalize(stmt);
    }

    return traffic;
}

} // namespace ntmd