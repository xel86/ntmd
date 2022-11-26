#include "PacketHash.hpp"
#include "net/Packet.hpp"

namespace ntmd {

PacketHash::PacketHash(const Packet& pkt)
{
    /* The /proc/net table has the socket information listed in local/remote format, so to associate
     * both incoming and outcoming packets to the same socket we have to reorder the source and
     * destination ip/port.*/
    if (pkt.direction == Direction::Outgoing)
    {
        this->ip1 = pkt.sip;
        this->port1 = pkt.sport;
        this->ip2 = pkt.dip;
        this->port2 = pkt.dport;
    }
    else
    {
        this->ip1 = pkt.dip;
        this->port1 = pkt.dport;
        this->ip2 = pkt.sip;
        this->port2 = pkt.sport;
    }
}

} // namespace ntmd