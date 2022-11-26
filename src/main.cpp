#include "config/ArgumentParser.hpp"
#include "config/Config.hpp"
#include "net/Sniffer.hpp"
#include "proc/ProcessIndex.hpp"

#include <iostream>

using namespace ntmd;

int main(int argc, char** argv)
{
    ArgumentParser args(argc, argv);

    Config cfg(args.configPath);
    cfg.mergeArgs(args);

    Sniffer sniffer(cfg);
}