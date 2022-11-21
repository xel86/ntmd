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
                     "Writing default config to "
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
        if (line.empty() || line[0] == '#')
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
    }
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
    cfg << "#Interval in seconds at which buffered network traffic in memory will be deposited to "
           "database.\n";
    cfg << "#Must be an integer value.\n"
        << "interval = " << this->interval << "\n";

    configFile << cfg.str();
}

} // namespace ntmd