#include "DBConnector.hpp"
#include "util/FilesystemUtil.hpp"
#include "util/StringUtil.hpp"

#include <filesystem>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <unistd.h>
#include <unordered_map>

namespace ntmd {

DBConnector::DBConnector(std::filesystem::path dbPath)
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

DBConnector::~DBConnector() { sqlite3_close(mHandle); }

void DBConnector::insertApplicationTraffic(
    const std::unordered_map<std::string, TrafficLine>& traffic)
{
    char* err;
    sqlite3_exec(mHandle, "BEGIN TRANSACTION", nullptr, nullptr, &err);

    char sqlCreateTable[512] = "CREATE TABLE IF NOT EXISTS %s ("
                               "timestamp INT PRIMARY KEY NOT NULL, "
                               "bytesRx INT DEFAULT 0, "
                               "bytesTx INT DEFAULT 0, "
                               "pktRxCount INT DEFAULT 0, "
                               "pktTxCount INT DEFAULT 0);";

    char sqlInsertValues[256] = "INSERT INTO %s VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt;
    time_t timestamp = std::time(nullptr);

    // TODO: Can this be done faster/prettier?
    for (const auto& [name, line] : traffic)
    {
        if (!util::isAlphanumeric(name))
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

        sqlite3_prepare_v3(mHandle, sqlReplaced, 512, 0, &stmt, NULL);
        int ret = sqlite3_step(stmt);
        if (ret != SQLITE_DONE)
        {
            std::cerr << "Commit failed while trying to create table.\n";
        }

        sqlite3_reset(stmt);

        snprintf(sqlReplaced, 512, sqlInsertValues, name.c_str());
        sqlite3_prepare_v3(mHandle, sqlReplaced, 512, 0, &stmt, NULL);
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

} // namespace ntmd