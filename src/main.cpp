#include "config/ArgumentParser.hpp"
#include "config/Config.hpp"
#include "net/Sniffer.hpp"
#include "proc/ProcessIndex.hpp"

#include <iostream>
#include <unistd.h>

using namespace ntmd;

int main(int argc, char** argv)
{
    if (geteuid() != 0)
    {
        std::cerr
            << "ntmd must be run as the root to sniff packets. Considering running using sudo.\n";
        std::exit(1);
    }
    ArgumentParser args(argc, argv);

    Config cfg(args.configPath);
    cfg.mergeArgs(args);

    Sniffer sniffer(cfg);
}