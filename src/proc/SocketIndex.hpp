#pragma once

#include "net/PacketHash.hpp"

#include <cstdint>
#include <mutex>
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

    /* For packets and their socket inodes that we cannot find a corresponding proc net line for,
     * add them to a not found list so that we don't continously hammer the CPU trying to find a
     * proc net line that we already know we can't find for every additional packet sniffed. Idealy
     * this list should be empty or very small. */
    std::mutex mMutex;
    std::unordered_map<PacketHash, bool> mCouldNotFind;
};

} // namespace ntmd