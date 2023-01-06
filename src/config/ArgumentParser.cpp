#include "ArgumentParser.hpp"
#include "Daemon.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ntmd {

const char* helpString =
    R"(Usage: ntmd [OPTIONS] ...
Network Traffic Monitoring Daemon
Example: ntmd -p 123 --config /home/user/ntmd.conf

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
    const std::vector<std::string_view> arg_list(argv + 1, argv + argc);
    for (auto it = arg_list.begin(), end = arg_list.end(); it != end; ++it)
    {
        std::string_view arg{*it};

        if (arg == "-h" || arg == "--help")
        {
            std::cout << helpString << std::endl;
            std::exit(0);
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
                    std::cerr << ntmd::logerror
                              << "The interval argument (-i, --interval) requires a "
                                 "32 bit integer in seconds. Invalid argument: "
                              << ex.what() << "\n";
                    std::exit(2);
                }
            }
            else
            {
                std::cerr << ntmd::logerror
                          << "The interval argument (-i, --interval) requires an "
                             "integer in seconds.\n";
                std::exit(2);
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
                    std::cerr << ntmd::logerror
                              << "The port argument (-p, --port) requires a "
                                 "16 bit unsigned integer in seconds. Invalid argument: "
                              << ex.what() << "\n";
                    std::exit(2);
                }
            }
            else
            {
                std::cerr << ntmd::logerror
                          << "The port argument (-p, --port) requires a "
                             "16 bit unsigned integer in seconds.\n";
                std::exit(2);
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
                    std::cerr << ntmd::logerror << "A config file at path " << this->configPath
                              << " does not exist.\n";
                    std::exit(2);
                }
            }
            else
            {
                std::cerr << ntmd::logerror
                          << "The config argument (-c, --config) requires an "
                             "absolute path to a config file.\n";
                std::exit(2);
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
                    << ntmd::logerror
                    << "The interface argument (--interface) requires a string value indiciating "
                       "the network interface for ntmd to monitor traffic from. (Example: eth0).\n";
                std::exit(2);
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
                    std::cerr << ntmd::lognotice << "A database file at path "
                              << this->dbPath.value().c_str()
                              << " does not exist, it will be created.\n";
                }
            }
            else
            {
                std::cerr << ntmd::logerror
                          << "The database argument (-c, --config) requires an "
                             "absolute path to a database file.\n";
                std::exit(2);
            }

            it++;
            continue;
        }

        /* Provided arg doesn't match any actual arguments */
        std::cerr << ntmd::logerror << "Invalid argument: " << arg
                  << ". Use --help to view list of valid arguments.\n";
        std::exit(2);
    }
}

ArgumentParser::~ArgumentParser() {}

} // namespace ntmd