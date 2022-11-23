#pragma once

#include <cstdint>
#include <ctime>
#include <iostream>

#include <pcap.h>

namespace ntmd {

enum class Direction
{
    Unknown,
    Incoming,
    Outgoing,
    NotOurs,
};

struct Packet
{
    Packet(const pcap_pkthdr* header, const u_char* rawPkt);
    ~Packet() = default;

    friend std::ostream& operator<<(std::ostream& os, const Packet& pkt);

    uint8_t protocol{0};                     /* Transport protocol used (tcp, udp, ...) */
    uint32_t sip{0};                         /* Source ip address */
    uint32_t dip{0};                         /* Destination ip address */
    uint16_t sport{0};                       /* Source port */
    uint16_t dport{0};                       /* Destination port */
    int len{0};                              /* Total length of packet */
    int totalHeaderLen{0};                   /* Total length of all headers */
    Direction direction{Direction::Unknown}; /* Direction of the packet relative to this machine */
    std::time_t timestamp;                   /* Unix timestamp when packet was captured */

    /* Should we discard this packet based on the information parsed? */
    bool discard{false};
};

} // namespace ntmd