#include "DBConnector.hpp"
#include "util/FilesystemUtil.hpp"

#include <filesystem>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <unistd.h>

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

    std::cerr << "Opening or creating database file at: " << dbPath << "\n";
    int error = sqlite3_open(dbPath.c_str(), &mHandle);
    if (error)
    {
        std::cerr << "Error opening or creating database at " << dbPath
                  << " with error: " << sqlite3_errmsg(mHandle) << "\n";
        std::cerr << "Cannot proceed without database connection, exiting.\n";
        exit(1);
    }
}

DBConnector::~DBConnector() { sqlite3_close(mHandle); }

} // namespace ntmd