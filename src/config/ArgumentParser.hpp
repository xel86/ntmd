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

    /* All arguments and their defaults */
    std::optional<bool> daemon;
    std::optional<bool> debug;
    std::optional<int> interval;
    std::optional<std::filesystem::path> configPath;
};

} // namespace ntmd