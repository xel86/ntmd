#pragma once

#include <filesystem>
#include <optional>

namespace ntmd {

/* Parses and stores the argument values given at program execution. */
class ArgumentParser
{
  public:
    ArgumentParser(int argc, char** argv);
    ~ArgumentParser();

    /* Command line arguments not present in the config file. */
    bool daemon{false};
    bool debug{false};
    std::filesystem::path configPath{};

    /* Command line arguments that have analogs to config file items.
     * Args set here will take precedence over the config file items. */
    std::optional<int> interval;
    std::optional<std::string> interface;
    std::optional<std::filesystem::path> dbPath;
    std::optional<uint16_t> serverPort;
};

} // namespace ntmd