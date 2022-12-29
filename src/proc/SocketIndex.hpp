#pragma once

#include "net/PacketHash.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ntmd {

struct Socket
{
    uint32_t localIP{0};
    uint32_t remoteIP{0};
    uint16_t localPort{0};
    uint16_t remotePort{0};
    uint64_t inode{0};
};

/* Index for all sockets in the /proc/net tables. Such as /proc/net/tcp.
 * The inode's listed for each socket in the tables are essential for
 * connecting them with the process that owns them. */
class SocketIndex
{
    using inode = uint64_t;

  public:
    SocketIndex();
    ~SocketIndex() = default;

    /* Update mSocketMap to be in sync with these /proc/net tables:
     * /proc/net/tcp
     * /proc/net/tcp6
     * /proc/net/udp
     * /proc/net/udp6
     * /proc/net/raw
     * /proc/net/raw6
     */
    void refresh(const std::vector<std::string>& tables);

    /* Gets the packet hash from the packet's local and remote ip/port values then
     * attempts to find a corresponding socket and its inode. */
    inode get(const Packet& pkt);

  private:
    std::unordered_map<PacketHash, inode> mSocketMap;
};

} // namespace ntmd