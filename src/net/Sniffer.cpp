#include "Sniffer.hpp"

#include <cstring>
#include <iostream>
#include <pcap/pcap.h>
#include <string>

#include <pcap.h>

namespace ntmd {

Sniffer::Sniffer(const std::string& device)
{
    findDevice(device);
    // TODO: Log device chosen to be monitored.
    std::cerr << "Chosen device: " << mDevice->name << "\n";

    /* TODO: Temporary settings! Investigate each individual setting and their impact.
     * Which ones should be configurable by user? */
    int promiscuous = 0;
    int immediate = 1;
    int timeoutLimit = 100; // milliseconds
    char errorBuffer[PCAP_ERRBUF_SIZE];

    mHandle = pcap_create(mDevice->name, errorBuffer);
    if (mHandle == nullptr)
    {
        std::cerr << "Could not open network device interface " << mDevice->name
                  << " for monitoring. Error: " << errorBuffer << "\n";
        std::exit(1);
    }

    if (pcap_set_timeout(mHandle, timeoutLimit))
        std::cerr << "Error setting packet buffer timeout, proceeding with it as instant.\n";

    if (pcap_set_immediate_mode(mHandle, immediate))
        std::cerr << "Error setting handle into immediate mode, proceeding with it off.\n";

    if (pcap_set_promisc(mHandle, promiscuous))
        std::cerr << "Error setting handle into promiscuous mode, proceeding with it off.\n";

    // TODO: Handle all warnings?
    if (pcap_activate(mHandle))
    {
        std::cerr << "Error activating configured pcap handle, cannot proceed. Error: "
                  << errorBuffer << "\n";
        std::exit(1);
    }
}

void Sniffer::findDevice(const std::string& device)
{
    char errorBuffer[PCAP_ERRBUF_SIZE];
    if (pcap_findalldevs(&mDevices, errorBuffer))
    {
        std::cerr << "Could not find any network devices to use. Error: " << errorBuffer << "\n";
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
        std::cerr << "Could not find specified interface device \"" << device
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