#include "IPList.hpp"

#include <cstring>
#include <ifaddrs.h>
#include <iostream>
#include <sys/types.h>
#include <vector>

#include <pcap.h>

namespace ntmd {

void IPList::init(const pcap_if* device)
{
    ifaddrs *interfaces, *interface;
    if (getifaddrs(&interfaces) < 0)
    {
        std::cerr << "Unable to access local interface addresses from idaddrs for " << device->name
                  << ". Cannot proceed, exiting.";
        std::exit(1);
    }

    std::vector<uint32_t> tmp;
    for (interface = interfaces; interface != nullptr; interface = interface->ifa_next)
    {
        if (interface->ifa_addr == nullptr)
            continue;

        if (strcmp(interface->ifa_name, device->name) != 0)
            continue;

        in_addr ip;
        ip.s_addr = ((sockaddr_in*)interface->ifa_addr)->sin_addr.s_addr;

        std::cerr << "Local IP address found for device " << device->name << ": " << inet_ntoa(ip)
                  << "\n";

        /* If address starts with 192.168.x.x push to the front of the list.
         * It will be assumed this will be the ip most searched for. */
        if (ip.s_addr >= 43200)
            tmp.insert(tmp.begin(), 1, ip.s_addr);
        else
            tmp.push_back(ip.s_addr);
    }

    for (int i = 0; i < tmp.size(); i++)
    {
        mIPs[i] = tmp[i];
        mSize++;
    }

    freeifaddrs(interfaces);
}

bool IPList::contains(uint32_t ip) const
{
    if (mIPs[0] == ip)
        return true;

    for (int i = 1; i < mSize; i++)
    {
        if (mIPs[i] == ip)
            return true;
    }

    return false;
}

} // namespace ntmd