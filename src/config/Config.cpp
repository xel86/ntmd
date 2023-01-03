#include "Config.hpp"

#include "util/FilesystemUtil.hpp"
#include "util/StringUtil.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <unordered_map>

namespace ntmd {

Config::Config(std::filesystem::path overridedPath)
{
    std::filesystem::path home = util::getHomeDirectory();

    /* If running as root user, make / the base directory.
     * Sometimes $HOME for root is set to /root/.
     */
    if (geteuid() == 0)
    {
        home.clear();
    }

    /* Preallocated preferred directory choices (this is the cleanest solution I could find). */
    std::filesystem::path first, second, third;
    if (!home.empty())
    {
        first = std::filesystem::path(home).append(".ntmdconf");
        second = std::filesystem::path(home).append(".config/ntmd/ntmd.conf");
    }
    else if (geteuid() != 0)
    {
        std::cerr << "Could not get location of home directory from $XDG_CONFIG_HOME or $HOME. "
                     "Falling back to root directory config location /etc/ntmd.conf.\n";
    }

    third = "/etc/ntmd.conf";

    if (!overridedPath.empty())
    {
        /* Path is verified to be valid in ArgumentParser.cpp */
        mFilePath = overridedPath;
    }
    else if (std::filesystem::is_regular_file(first))
    {
        mFilePath = first;
    }
    else if (std::filesystem::is_regular_file(second))
    {
        mFilePath = second;
    }
    else if (std::filesystem::is_regular_file(third))
    {
        mFilePath = third;
    }

    if (mFilePath.empty())
    {
        if (!home.empty())
        {
            mFilePath = home.append(".ntmdconf");
        }
        else
        {
            mFilePath = "/etc/ntmd.conf";
        }

        std::cerr << "No config file present in ~/.ntmdconf, ~/.config/ntmd/ntmd.conf, or "
                     "/etc/ntmd.conf.\n"
                     "Using default values and writing default config to "
                  << mFilePath << "\n";

        this->writeConfig();
        return;
    }

    std::ifstream configFile(mFilePath);
    if (!configFile.is_open())
    {
        std::cerr << "Couldn't open config file " << mFilePath << "\n";
        std::exit(1);
    }

    std::unordered_map<std::string, std::string> items;
    std::string line;
    while (std::getline(configFile, line))
    {
        if (line.empty() || line[0] == '#' || line[0] == '[')
        {
            continue;
        }

        auto delimPos = line.find("=");
        if (delimPos != std::string::npos)
        {
            auto key = util::trim(line.substr(0, delimPos));
            auto value = util::trim(line.substr(delimPos + 1));

            items[key] = value;
            continue;
        }
    }

    if (items.count("interval"))
    {
        try
        {
            this->interval = std::stoi(items["interval"]);
        }
        catch (std::invalid_argument& ia)
        {
            std::cerr
                << "Config item \"interval\" is attempting to be set with a non-integer value (\""
                << items["interval"] << "\"). Defaulting to " << this->interval << "\n";
        }
        catch (std::out_of_range& oor)
        {
            std::cerr << "Config item \"interval\" is attempting to be set with an integer value "
                         "too large (\""
                      << items["interval"] << "\"). Defaulting to " << this->interval << "\n";
        }
    }

    if (items.count("interface"))
    {
        this->interface = items["interface"];
    }

    if (items.count("promiscuous"))
    {
        const std::string& val = util::strToLower(items["promiscuous"]);

        try
        {
            this->promiscuous = util::stringToBool(val);
        }
        catch (std::invalid_argument& ia)
        {
            std::cerr << "Config item \"promiscuous\" is attempting to be set with a non-boolean "
                         "value (\""
                      << items["promiscuous"] << "\"). Defaulting to " << this->promiscuous << "\n";
        }
    }

    if (items.count("immediate"))
    {
        const std::string& val = util::strToLower(items["immediate"]);
        try
        {
            this->immediate = util::stringToBool(val);
        }
        catch (std::invalid_argument& ia)
        {
            std::cerr << "Config item \"immediate\" is attempting to be set with a non-boolean "
                         "value (\""
                      << items["immediate"] << "\"). Defaulting to " << this->immediate << "\n";
        }
    }

    if (items.count("dbPath"))
    {
        this->dbPath = items["dbPath"];
        if (!std::filesystem::is_regular_file(this->dbPath))
        {
            fprintf(stderr, "A database file at path \"%s\" does not exist, it will be created.\n",
                    this->dbPath.c_str());
        }
    }

    if (items.count("port"))
    {
        try
        {
            this->serverPort = std::stoi(items["port"]);
        }
        catch (std::invalid_argument& ia)
        {
            std::cerr << "Config item \"port\" is attempting to be set with a non-integer value (\""
                      << items["port"] << "\"). Defaulting to " << this->serverPort << "\n";
        }
        catch (std::out_of_range& oor)
        {
            std::cerr << "Config item \"port\" is attempting to be set with an integer value too "
                         "large (\""
                      << items["port"] << "\"). Defaulting to " << this->serverPort << "\n";
        }
    }

    if (items.count("processCacheSize"))
    {
        try
        {
            this->processCacheSize = std::stoi(items["processCacheSize"]);
        }
        catch (std::invalid_argument& ia)
        {
            std::cerr << "Config item \"processCacheSize\" is attempting to be set with a "
                         "non-integer value (\""
                      << items["processCacheSize"] << "\"). Defaulting to "
                      << this->processCacheSize << "\n";
        }
        catch (std::out_of_range& oor)
        {
            std::cerr << "Config item \"processCacheSize\" is attempting to be set with an "
                         "integer value too large (\""
                      << items["processCacheSize"] << "\"). Defaulting to "
                      << this->processCacheSize << "\n";
        }
    }

    /* This will ensure the config is up to date after adding new config items.
     * Keeps current config values and adds new fields with their defaults. */
    this->writeConfig();
}

void Config::mergeArgs(ArgumentParser& args)
{
    this->interval = args.interval.value_or(this->interval);
    this->interface = args.interface.value_or(this->interface);
    this->dbPath = args.dbPath.value_or(this->dbPath);
    this->serverPort = args.serverPort.value_or(this->serverPort);
}

void Config::writeConfig()
{
    std::ofstream configFile(mFilePath);
    if (!configFile.is_open())
    {
        std::cerr << "Couldn't open default config file for writing at path " << mFilePath << "\n";
        std::exit(1);
    }

    std::stringstream cfg;
    cfg << "#ntmd generated config file.\n\n";

    cfg << "[ntmd]\n\n";
    cfg << "#Interval in seconds at which buffered network traffic in memory will be deposited to "
           "database.\n";
    cfg << "#Must be an integer value.\n"
        << "interval = " << this->interval << "\n\n";
    cfg << "#Size for the process index LRU cache, leave default if unsure.\n";
    cfg << "#If you expect for many longrunning programs to use sockets frequently, increase the "
           "cache size to include those programs.\n";
    cfg << "#If you expect for many new programs to spawn, create sockets, then close (per "
           "second), you may "
           "want a lower cache size or none at all (0).\n";
    cfg << "#In most cases the default will be good for both situations.\n";
    cfg << "processCacheSize = " << this->processCacheSize << "\n";

    cfg << "\n";

    cfg << "[network]\n\n";
    cfg << "#Network interface to be search for for ntmd to monitor traffic on. If value left "
           "empty ntmd will use the first device found.\n";
    cfg << "#An example network interface could be eth0\n";
    cfg << "interface = " << this->interface << "\n";

    cfg << "\n";

    cfg << "[pcap]\n\n";
    cfg << "#Enable or disable promiscuous mode while sniffing packets.\n";
    cfg << "promiscuous = " << (this->promiscuous ? "true" : "false") << "\n\n";
    cfg << "#Enable or disable immediate mode while sniffing packets.\n";
    cfg << "#Immediate mode turned on will greatly increase average CPU usage but may decrease the "
           "amount of unmatched packets.\n";
    cfg << "immediate = " << (this->immediate ? "true" : "false") << "\n";

    cfg << "\n";
    cfg << "[database]\n\n";
    cfg << "#Path to the database file that will be used for reading and writing application "
           "network traffic.\n";
    cfg << "#If left empty the default for root is /var/lib/ntmd.db and non-root is ~/.ntmd.db.\n";
    cfg << "dbPath = " << this->dbPath.string() << "\n";

    cfg << "\n";

    cfg << "[api]\n\n";
    cfg << "#Port for socket server to be hosted on (16 bit unsigned).\n";
    cfg << "port = " << static_cast<int>(this->serverPort) << "\n";

    configFile << cfg.str();
}

} // namespace ntmd