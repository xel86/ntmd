#include "Packet.hpp"
#include "Sniffer.hpp"
#include "proc/ProcessIndex.hpp"

#include <pcap.h>

namespace ntmd {

void SnifferLoop::pktCallback(u_char* user, const pcap_pkthdr* hdr, const u_char* rawPkt)
{
    Sniffer* s = reinterpret_cast<Sniffer*>(user);

    Packet pkt(hdr, rawPkt, s->mIPList);

    /* We will discard packets we don't care about in the future.
     * For now lets see all of them for debugging. */
    if (pkt.discard)
        return;

    auto process = s->mProcessResolver.resolve(pkt);

    if (process.has_value())
        s->mTrafficStorage.add(process->get(), pkt);
}

} // namespace ntmd