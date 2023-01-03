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
  -p, --port        Port for the socket server to be hosted on.
  -c, --config      Absolute path to search for the config file location rather than defaults.
  --interface       Network interface for ntmd to monitor traffic on (example: eth0).
  --db-path         Path to database file to be used for reading and writing traffic.
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
                catch (const std::exception& ex)
                {
                    fprintf(stderr,
                            "The interval argument (-i, --interval) requires a "
                            "32 bit integer in seconds. Invalid argument: %s\n",
                            ex.what());
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

        if (arg == "-p" || arg == "--port")
        {
            if (it + 1 != end)
            {
                try
                {
                    this->serverPort = std::stoi(std::string(*(it + 1)));
                }
                catch (const std::exception& ex)
                {
                    fprintf(stderr,
                            "The port argument (-p, --port) requires a "
                            "16 bit unsigned integer in seconds. Invalid argument: %s\n",
                            ex.what());
                    exit(1);
                }
            }
            else
            {
                fprintf(stderr, "The port argument (-p, --port) requires a "
                                "16 bit unsigned integer in seconds.\n");
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
                    fprintf(stderr, "A config file at path \"%s\" does not exist.\n",
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

        if (arg == "--interface")
        {
            if (it + 1 != end)
            {
                this->interface = *(it + 1);
            }
            else
            {
                std::cerr
                    << "The interface argument (--interface) requires a string value indiciating "
                       "the network interface for ntmd to monitor traffic from. (Example: eth0).\n";
                exit(1);
            }

            it++;
            continue;
        }

        if (arg == "--db-path")
        {
            if (it + 1 != end)
            {
                this->dbPath = *(it + 1);
                if (!std::filesystem::is_regular_file(this->dbPath.value()))
                {
                    fprintf(stderr,
                            "A database file at path \"%s\" does not exist, it will be created.\n",
                            this->dbPath.value().c_str());
                }
            }
            else
            {
                fprintf(stderr, "The database argument (-c, --config) requires an "
                                "absolute path to a database file.\n");
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