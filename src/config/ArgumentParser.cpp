#include "ArgumentParser.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ntmd {

const char* helpString =
    R"(Usage: ntmd [OPTIONS] ...
Network Traffic Monitoring Daemon
Example: ntmd --daemon

Arguments:
  -x, --debug       Print additional debug information to log.
  -i, --interval    Interval in seconds to update database with buffered traffic.
  -c, --config      Absolute path to search for the config file location rather than defaults.
  --daemon          Used to launch initial daemon process to monitor traffic.
)";

ArgumentParser::ArgumentParser(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << helpString << std::endl;
        exit(1);
    }

    const std::vector<std::string_view> arg_list(argv + 1, argv + argc);
    for (auto it = arg_list.begin(), end = arg_list.end(); it != end; ++it)
    {
        std::string_view arg{*it};

        if (arg == "-h" || arg == "--help")
        {
            std::cout << helpString << std::endl;
            exit(1);
        }

        if (arg == "--daemon")
        {
            this->daemon = true;
            continue;
        }

        if (arg == "-x" || arg == "--debug")
        {
            this->debug = true;
            continue;
        }

        if (arg == "-i" || arg == "--interval")
        {
            if (it + 1 != end)
            {
                try
                {
                    this->interval = std::stoi(std::string(*(it + 1)));
                }
                catch (const std::invalid_argument& ia)
                {
                    fprintf(stderr,
                            "The interval argument (-i, --interval) requires an "
                            "integer in seconds. Invalid argument: %s\n",
                            ia.what());
                    exit(1);
                }
            }
            else
            {
                fprintf(stderr, "The interval argument (-i, --interval) requires an "
                                "integer in seconds.\n");
                exit(1);
            }

            it++;
            continue;
        }

        if (arg == "-c" || arg == "--config")
        {
            if (it + 1 != end)
            {
                this->configPath = *(it + 1);
                if (!std::filesystem::is_regular_file(this->configPath))
                {
                    fprintf(stderr, "A config file at path \"%s\" does not exist.",
                            this->configPath.c_str());
                    exit(1);
                }
            }
            else
            {
                fprintf(stderr, "The config argument (-c, --config) requires an "
                                "absolute path to a config file.\n");
                exit(1);
            }

            it++;
            continue;
        }

        /* Provided arg doesn't match any actual arguments */
        std::cout << "Invalid argument: " << arg << "\n\n";
        std::cout << helpString << std::endl;
        exit(1);
    }
}

ArgumentParser::~ArgumentParser() {}

} // namespace ntmd