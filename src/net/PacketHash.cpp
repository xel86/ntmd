#include "PacketHash.hpp"
#include "net/Packet.hpp"

namespace ntmd {

PacketHash::PacketHash(const Packet& pkt)
{
    /* The /proc/net table has the socket information listed in local/remote format, so to associate
     * both incoming and outcoming packets to the same socket we have to reorder the source and
     * destination ip/port.
     * UDP Packet hashes only have the local port incorporated in its packet hash.
     */
    if (pkt.direction == Direction::Outgoing)
    {
        this->port1 = pkt.sport;
        if (pkt.type == PacketType::UDP)
            return;

        this->ip1 = pkt.sip;
        this->ip2 = pkt.dip;
        this->port2 = pkt.dport;
    }
    else
    {
        this->port1 = pkt.dport;
        if (pkt.type == PacketType::UDP)
            return;

        this->ip1 = pkt.dip;
        this->ip2 = pkt.sip;
        this->port2 = pkt.sport;
    }
}

} // namespace ntmd