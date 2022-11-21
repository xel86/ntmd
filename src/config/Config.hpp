#pragma once

#include <filesystem>

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
    Config(std::filesystem::path overridePath = {});

    ~Config() = default;

    /* Writes config file to mFilePath using all the values currently set in the config class.
     * Using this upon class creation will write a default config to the default path. */
    void writeConfig();

    /* Interval in seconds at which buffered network traffic in memory will be deposited to db */
    int interval{10};

  private:
    std::filesystem::path mFilePath{};
};

} // namespace ntmd