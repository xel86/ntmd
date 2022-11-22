#pragma once

#include "config/Config.hpp"

#include <string>

#include <pcap.h>

namespace ntmd {

class Sniffer
{
  public:
    Sniffer(const Config& cfg);
    ~Sniffer();

    /* Trys to find and set the device given or if the device parameter
     * is empty will use the first device found. */
    void findDevice(const std::string& device);

  private:
    pcap_if* mDevice{nullptr};
    pcap_if_t* mDevices{nullptr};
    pcap_t* mHandle{nullptr};
};

} // namespace ntmd