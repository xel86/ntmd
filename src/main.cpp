#include "cli/ArgumentParser.hpp"

#include <iostream>

using namespace ntmd;

int main(int argc, char** argv)
{
    ArgumentParser args(argc, argv);

    std::cout << "Daemon: " << args.daemon << " Debug: " << args.debug
              << " Interval: " << args.interval << std::endl;
}