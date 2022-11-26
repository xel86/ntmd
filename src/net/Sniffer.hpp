#pragma once

#include "IPList.hpp"
#include "config/Config.hpp"
#include "proc/ProcessResolver.hpp"

#include <string>

#include <pcap.h>

namespace ntmd {

/* Kind of hacky solution to getting the static pktCallback access to the private member variables
 * of Sniffer. Required since you can't declare a friend static method directly. */
struct SnifferLoop
{
    friend class Sniffer;
    static void pktCallback(u_char* user, const pcap_pkthdr* hdr, const u_char* bytes);
};

class Sniffer
{
  public:
    Sniffer(const Config& cfg);
    ~Sniffer();

    /* Trys to find and set the device given or if the device parameter
     * is empty will use the first device found. */
    void findDevice(const std::string& device);

    friend void SnifferLoop::pktCallback(u_char* user, const pcap_pkthdr* hdr, const u_char* bytes);

  private:
    IPList mIPList;
    ProcessResolver mProcessResolver;

    pcap_if* mDevice{nullptr};
    pcap_if_t* mDevices{nullptr};
    pcap_t* mHandle{nullptr};
};

} // namespace ntmd