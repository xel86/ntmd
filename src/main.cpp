#include "config/ArgumentParser.hpp"
#include "config/Config.hpp"

#include <iostream>

using namespace ntmd;

int main(int argc, char** argv)
{
    ArgumentParser args(argc, argv);

    std::cout << "Daemon: " << args.daemon << " Debug: " << args.debug
              << " Interval: " << args.interval << std::endl;

    Config cfg(args.configPath);
    std::cout << "Config interval value: " << cfg.interval << std::endl;
}