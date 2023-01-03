#pragma once

#include "ArgumentParser.hpp"

#include <filesystem>
#include <string>

namespace ntmd {

/* Represents the config file and its values.
 * Paths are searched for in this order if no override is given:
 * ~/.ntmdconf
 * ~/.config/ntmd/ntmd.conf
 * /etc/ntmd.conf
 */
class Config
{
  public:
    Config(std::filesystem::path overridePath);

    ~Config() = default;

    /* Merges in the command line arguments to replace/update values from the config file. */
    void mergeArgs(ArgumentParser& args);

    /* Writes config file to mFilePath using all the values currently set in the config class.
     * Using this upon class creation will write a default config to the default path. */
    void writeConfig();

    /* All config options */

    /* Interval in seconds at which buffered network traffic in memory will be deposited to db. */
    int interval{10};
    /* Network interface for pcap to use instead of the default. */
    std::string interface {};
    /* PCAP promiscuous mode. */
    bool promiscuous{false};
    /* PCAP immediate mode (much higher CPU usage but will potentially match more packets). */
    bool immediate{false};
    /* Database path for traffic reading and writing.
     * If we are root, default is /var/lib/ntmd.db
     * If we are not-root, default is ~/.ntmd.db */
    std::filesystem::path dbPath{};

    /* ProcessIndex mLRUCache size. The larger the size, the longer it will take for the
     * ProcessIndex to find processes not in cache. The smaller the size, the easier it will be to
     * find new processes but slower to find existing processes that create sockets regularly (this
     * is more typical). Default of 5 is a good middle ground for both. */
    int processCacheSize{5};

    /* Port for the API socket server to be hosted on. */
    uint16_t serverPort{13889};

  private:
    std::filesystem::path mFilePath{};
};

} // namespace ntmd