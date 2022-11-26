#include "SocketIndex.hpp"
#include "net/PacketHash.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace ntmd {

using inode = uint64_t;

SocketIndex::SocketIndex() { refresh(); }

void SocketIndex::refresh()
{
    const std::vector<std::string> tables = {"/proc/net/tcp", "/proc/net/udp", "/proc/net/raw"};
    for (const std::string& table : tables)
    {
        std::ifstream fs(table);
        if (!fs.is_open())
        {
            std::cerr << "Failed to open /proc socket table " << table
                      << ". Socket Index will not be properly updated resulting in packets not "
                         "being matched.\n";
            continue;
        }

        std::string line;
        std::getline(fs, line); // skip header line

        while (std::getline(fs, line))
        {
            /* Unpack a single line from the proc table, this represents a single open socket. */
            Socket sock;

            char packedLocalIP[65], packedRemoteIP[65];
            int matches = sscanf(line.c_str(),
                                 "%*d: %64[0-9A-Fa-f]:%hX %64[0-9A-Fa-f]:%hX %*X "
                                 "%*X:%*X %*X:%*X %*X %*d %*d %ld %*512s\n",
                                 packedLocalIP, &sock.localPort, packedRemoteIP, &sock.remotePort,
                                 &sock.inode);

            if (matches != 5)
            {
                std::cerr << "Malformed line buffer from a /proc/net line in socket table " << table
                          << ".\n";
                continue;
            }

            /* Don't update this socket line if it is in TIME_WAIT state. */
            if (sock.inode == 0)
                continue;

            /* Unpack encoded local/remote ip strings from socket line. */
            sscanf(packedLocalIP, "%X", &sock.localIP);
            sscanf(packedRemoteIP, "%X", &sock.remoteIP);

            /* Create packet hash from socket information
             * for future sniffed packet to match against. */
            PacketHash hash(sock.localIP, sock.localPort, sock.remoteIP, sock.remotePort);

            mSocketMap[hash] = sock.inode;
        }
    }
}

inode SocketIndex::get(const Packet& pkt)
{
    PacketHash hash(pkt);
    const auto& found = mSocketMap.find(hash);
    if (found != mSocketMap.end())
    {
        return found->second;
    }
    else
    {
        std::cerr << "Could not find an associated inode from the packet hash!\n";
        return 0;
    }
}

} // namespace ntmd