#include "Packet.hpp"

#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

namespace ntmd {

Packet::Packet(const pcap_pkthdr* header, const u_char* rawPkt)
{
    // skip over ethernet header ( always 14 bytes ) and use ip header
    iphdr* ipHeader = (iphdr*)(rawPkt + sizeof(ethhdr));
    unsigned short ipHeaderLen = ipHeader->ihl * 4;

    this->len = header->len;
    this->timestamp = header->ts.tv_sec;
    this->sip = ipHeader->saddr;
    this->dip = ipHeader->daddr;
    this->protocol = ipHeader->protocol;

    /* TODO: Get packet direction based off local ip address. */

    int offset = (ipHeaderLen) + sizeof(ethhdr);
    switch (this->protocol)
    {
    case IPPROTO_TCP: {
        tcphdr* tcpHeader = (tcphdr*)(rawPkt + offset);
        this->totalHeaderLen = offset + sizeof(tcpHeader->doff * 4);
        this->sport = ntohs(tcpHeader->source);
        this->dport = ntohs(tcpHeader->dest);
    }
    break;

    case IPPROTO_UDP: {
        udphdr* udpHeader = (udphdr*)(rawPkt + offset);
        this->totalHeaderLen = offset + 8; // UDP Header always 8 bytes.
        this->sport = ntohs(udpHeader->source);
        this->dport = ntohs(udpHeader->dest);
    }
    break;

    case IPPROTO_ICMP: {
        icmphdr* icmpHeader = (icmphdr*)(rawPkt + offset);
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
    std::string protocolStr;
    switch (pkt.protocol)
    {
    case IPPROTO_TCP:
        protocolStr = "TCP";
        break;
    case IPPROTO_UDP:
        protocolStr = "UDP";
        break;
    case IPPROTO_ICMP:
        protocolStr = "ICMP";
        break;
    default:
        protocolStr = "Unknown (" + std::to_string(pkt.protocol) + ")";
        break;
    }

    os << protocolStr << " Packet: {\n";

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