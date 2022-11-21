#pragma once

#include <filesystem>

namespace ntmd {

/* Parses and stores the argument values given at program execution. */
class ArgumentParser
{
  public:
    ArgumentParser(int argc, char** argv);
    ~ArgumentParser();

    /* All arguments and their defaults */
    bool daemon{false};
    bool debug{false};
    int interval{10};
    std::filesystem::path configPath{};
};

} // namespace ntmd