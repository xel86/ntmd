#include "Packet.hpp"
#include "IPList.hpp"

#include <net/ethernet.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

namespace ntmd {

Packet::Packet(const pcap_pkthdr* header, const u_char* rawPkt, const IPList& iplist)
{
    /* Discard anything but IPV4 packets for now. */
    /* TODO: Support IPV6 */
    ether_header* eth_header = (ether_header*)rawPkt;
    if (ntohs(eth_header->ether_type) != ETHERTYPE_IP)
    {
        this->discard = true;
        return;
    }

    // skip over ethernet header ( always 14 bytes ) and use ip header
    iphdr* ipHeader = (iphdr*)(rawPkt + sizeof(ethhdr));
    unsigned short ipHeaderLen = ipHeader->ihl * 4;

    this->len = header->len;
    this->timestamp = header->ts.tv_sec;
    this->sip = ipHeader->saddr;
    this->dip = ipHeader->daddr;
    this->protocol = ipHeader->protocol;

    if (iplist.contains(this->sip))
        this->direction = Direction::Outgoing;
    else if (iplist.contains(this->dip))
        this->direction = Direction::Incoming;
    else
    {
        this->direction = Direction::Unknown;
        this->discard = true;
        return;
    }

    int offset = (ipHeaderLen) + sizeof(ethhdr);
    switch (this->protocol)
    {
    case IPPROTO_TCP: {
        tcphdr* tcpHeader = (tcphdr*)(rawPkt + offset);
        this->type = PacketType::TCP;
        this->totalHeaderLen = offset + sizeof(tcpHeader->doff * 4);
        this->sport = ntohs(tcpHeader->source);
        this->dport = ntohs(tcpHeader->dest);
    }
    break;

    case IPPROTO_UDP: {
        udphdr* udpHeader = (udphdr*)(rawPkt + offset);
        this->type = PacketType::UDP;
        this->totalHeaderLen = offset + 8; // UDP Header always 8 bytes.
        this->sport = ntohs(udpHeader->source);
        this->dport = ntohs(udpHeader->dest);

        /* Further distinguish UDP packets */
        /* TODO: Make placeholder applications for these special packets */

        /* DNS & MDNS Packets.
         * https://stackoverflow.com/questions/7565300/identifying-dns-packets
         */
        if ((this->sport == 53 || this->dport == 53) ||
            (this->sport == 5353 && this->dport == 5353))
        {
            this->type = PacketType::DNS;
            this->discard = true;
        }

        /* SSDP packets.
         * https://wiki.wireshark.org/SSDP
         */
        if (this->sport == 1900 || this->dport == 1900)
        {
            this->type = PacketType::SSDP;
            this->discard = true;
        }

        /* NTP (Network Time Protocol) packets. */
        if (this->sport == 123 && this->dport == 123)
        {
            this->type = PacketType::NTP;
            this->discard = true;
        }
    }
    break;

    case IPPROTO_ICMP: {
        icmphdr* icmpHeader = (icmphdr*)(rawPkt + offset);
        this->type = PacketType::ICMP;
        this->totalHeaderLen = offset + 8; // ICMP Header always 8 bytes.

        /* ICMP has no ports. */
        this->sport = 0;
        this->dport = 0;
    }
    break;

    default: {
        this->discard = true;
        return;
    }
    break;
    }
}

std::ostream& operator<<(std::ostream& os, const Packet& pkt)
{
    std::string typeStr, directionStr;
    switch (pkt.type)
    {
    case PacketType::TCP:
        typeStr = "TCP";
        break;
    case PacketType::UDP:
        typeStr = "UDP";
        break;
    case PacketType::ICMP:
        typeStr = "ICMP";
        break;
    case PacketType::DNS:
        typeStr = "DNS";
        break;
    case PacketType::SSDP:
        typeStr = "SSDP";
        break;
    case PacketType::NTP:
        typeStr = "NTP";
        break;

    case PacketType::Unknown:
    default:
        typeStr = "Unknown (" + std::to_string(pkt.protocol) + ")";
        break;
    }

    switch (pkt.direction)
    {
    case Direction::Incoming:
        directionStr = "Incoming";
        break;
    case Direction::Outgoing:
        directionStr = "Outgoing";
        break;
    case Direction::Unknown:
        directionStr = "Unknown";
        break;
    }

    os << directionStr << " " << typeStr << " Packet: {\n";

    in_addr sAddr, dAddr;
    sAddr.s_addr = pkt.sip;
    dAddr.s_addr = pkt.dip;

    char sipStr[INET_ADDRSTRLEN], dipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(sAddr), sipStr, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(dAddr), dipStr, INET_ADDRSTRLEN);

    os << "  ";
    os << sipStr << ":" << static_cast<int>(pkt.sport);
    os << " -> ";
    os << dipStr << ":" << static_cast<int>(pkt.dport);
    os << "\n";

    os << "  ";
    os << "Len: " << pkt.len;
    os << "\n";

    os << "  ";
    os << "Time: " << pkt.timestamp;
    os << "\n";

    os << "}";

    return os;
}

} // namespace ntmd