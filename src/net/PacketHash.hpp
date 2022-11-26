#pragma once

#include "net/Packet.hpp"

#include <cstdint>

namespace ntmd {

/* Hash to identify similar packets by their local ip/port and remote ip/port. This is to be able to
 * identify the socket associated with sniffed packets since that information is listed in the
 * /proc/net/ table for said socket.*/
struct PacketHash
{
    PacketHash(const Packet& pkt);
    PacketHash(uint32_t localip, uint16_t localport, uint32_t remoteip, uint16_t remoteport) :
        ip1(localip), port1(localport), ip2(remoteip), port2(remoteport){};

    ~PacketHash() = default;

    uint32_t ip1;
    uint16_t port1;
    uint32_t ip2;
    uint16_t port2;

    bool operator==(const PacketHash& other) const
    {
        return (ip1 == other.ip1 && port1 == other.port1 && ip2 == other.ip2 &&
                port2 == other.port2);
    }
};

} // namespace ntmd

namespace std {
using ntmd::PacketHash;

template <>
struct hash<PacketHash>
{
    std::size_t operator()(const PacketHash& p) const
    {
        return ((hash<uint32_t>()(p.ip1) ^ (hash<uint16_t>()(p.port1) << 1)) >> 1) ^
               (hash<uint32_t>()(p.ip2) << 1) ^ (hash<uint16_t>()(p.port2) << 1) >> 1;
    }
};

} // namespace std