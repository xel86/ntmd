#include "api/APIController.hpp"
#include "config/ArgumentParser.hpp"
#include "config/Config.hpp"
#include "net/Sniffer.hpp"
#include "proc/ProcessIndex.hpp"
#include "traffic/DBController.hpp"
#include "traffic/TrafficStorage.hpp"

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

    /* Database controller that is the only object with direct access
     * in or out of the local database that stores all network traffic
     * monitored from ntmd. */
    auto db = DBController(cfg.dbPath);

    /* Traffic storage that stores the in-memory network traffic monitored from the sniffer before
     * it gets deposited into the database using the DBController. The in-memory traffic gets
     * deposited into the database on a set interval from the config and then gets cleared. */
    auto trafficStorage = TrafficStorage(cfg.interval, db);

    /* Socket API controller that manages the socket server to respond to incoming socket API
     * requests. Has a reference to both the traffic storage for peeking into a live view of
     * in-memory traffic, and the db controller for easy access to historical traffic data. */
    auto api = APIController(trafficStorage, db);

    Sniffer sniffer(cfg, trafficStorage);
}