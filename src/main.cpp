#include "config/ArgumentParser.hpp"
#include "config/Config.hpp"

#include <iostream>

using namespace ntmd;

int main(int argc, char** argv)
{
    ArgumentParser args(argc, argv);

    Config cfg(args.configPath.value_or(""));
    cfg.mergeArgs(args);

    std::cout << "Config interval value: " << cfg.interval << std::endl;
}