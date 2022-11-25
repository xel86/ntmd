#pragma once

#include <cstdint>
#include <unordered_map>

namespace ntmd {

struct Socket
{
    uint32_t localIP;
    uint32_t remoteIP;
    uint16_t localPort;
    uint16_t remotePort;
    uint64_t inode;
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
    void refresh();

  private:
    std::unordered_map<inode, Socket> mSocketMap;
};

} // namespace ntmd