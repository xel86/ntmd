#include "Sniffer.hpp"
#include "Daemon.hpp"
#include "IPList.hpp"
#include "config/Config.hpp"

#include <cstring>
#include <iostream>
#include <pcap/pcap.h>
#include <string>

#include <pcap.h>

namespace ntmd {

Sniffer::Sniffer(const Config& cfg, TrafficStorage& trafficStorage) :
    mTrafficStorage(trafficStorage), mProcessResolver(cfg.processCacheSize)
{
    const std::string& device = cfg.interface;
    const int promiscuous = cfg.promiscuous ? 1 : 0;
    const int immediate = cfg.immediate ? 1 : 0;

    findDevice(device);
    std::cerr << ntmd::loginfo << "Chosen network device: " << mDevice->name << "\n";

    /* TODO: Temporary settings! Investigate each individual setting and their impact.
     * Which ones should be configurable by user? */
    int timeoutLimit = 100; // milliseconds
    char errorBuffer[PCAP_ERRBUF_SIZE];

    mHandle = pcap_create(mDevice->name, errorBuffer);
    if (mHandle == nullptr)
    {
        std::cerr << ntmd::logerror << "Could not open network device interface " << mDevice->name
                  << " for monitoring. Error: " << errorBuffer << "\n";
        std::exit(1);
    }

    if (pcap_set_timeout(mHandle, timeoutLimit))
        std::cerr << ntmd::logwarn
                  << "Error setting packet buffer timeout, proceeding with it as instant.\n";

    if (pcap_set_immediate_mode(mHandle, immediate))
        std::cerr << ntmd::logwarn
                  << "Error setting handle into immediate mode, proceeding with it off.\n";

    if (pcap_set_promisc(mHandle, promiscuous))
        std::cerr << ntmd::logwarn
                  << "Error setting handle into promiscuous mode, proceeding with it off.\n";

    // TODO: Handle all warnings?
    if (pcap_activate(mHandle))
    {
        std::cerr << ntmd::logerror
                  << "Error activating configured pcap handle, cannot proceed. Error: "
                  << errorBuffer << "\n";
        std::exit(1);
    }

    mIPList.init(mDevice);
}

int Sniffer::dispatch()
{
    return pcap_dispatch(mHandle, -1, SnifferLoop::pktCallback, reinterpret_cast<u_char*>(this));
}

void Sniffer::findDevice(const std::string& device)
{
    char errorBuffer[PCAP_ERRBUF_SIZE];
    if (pcap_findalldevs(&mDevices, errorBuffer))
    {
        std::cerr << ntmd::logerror
                  << "Could not find any network devices to use. Error: " << errorBuffer << "\n";
        std::exit(1);
    }

    if (!device.empty())
    {
        for (pcap_if_t* i = mDevices; i != NULL; i = i->next)
        {
            // TODO: Log devices found probably in verbose mode
            if (std::strcmp(i->name, device.c_str()) == 0)
            {
                mDevice = i;
                break;
            }
        }
    }
    else
    {
        /* Use the first device in the list as the default if no
         * device specified in the config or as a command line argument. */
        mDevice = mDevices;
    }

    if (mDevice == nullptr)
    {
        std::cerr << ntmd::logerror << "Could not find specified interface device \"" << device
                  << "\", either correct the interface or use no value for ntmd to select the "
                     "first device found.\n";
        std::exit(1);
    }
}

Sniffer::~Sniffer()
{
    if (mHandle != nullptr)
        pcap_close(mHandle);

    if (mDevices != nullptr)
        pcap_freealldevs(mDevices);
}

} // namespace ntmd