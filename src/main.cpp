#include "config/ArgumentParser.hpp"
#include "config/Config.hpp"

#include <iostream>

using namespace ntmd;

int main(int argc, char** argv)
{
    ArgumentParser args(argc, argv);

    Config cfg(args.configPath);
    cfg.mergeArgs(args);

    std::cout << "daemon: " << args.daemon << "\n";
    std::cout << "debug: " << args.debug << "\n";
    std::cout << "configPath: " << args.configPath << "\n";
    std::cout << "interval: " << cfg.interval << "\n";
}