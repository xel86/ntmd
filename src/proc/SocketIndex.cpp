#include "SocketIndex.hpp"
#include "Daemon.hpp"
#include "net/PacketHash.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace ntmd {

using inode = uint64_t;

SocketIndex::SocketIndex()
{
    refresh({"/proc/net/tcp", "/proc/net/udp", "/proc/net/raw"});

    std::thread loop([this] {
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(60));

            std::unique_lock<std::mutex> lock(mMutex);
            mCouldNotFind.clear();
        }
    });
    loop.detach();
}

void SocketIndex::refresh(const std::vector<std::string>& tables)
{
    for (const std::string& table : tables)
    {
        std::ifstream fs(table);
        if (!fs.is_open())
        {
            std::cerr << ntmd::logwarn << "Failed to open /proc socket table " << table
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
                std::cerr << ntmd::logwarn
                          << "Malformed line buffer from a /proc/net line in socket table " << table
                          << ".\n";
                continue;
            }

            /* Don't update this socket line if it is in TIME_WAIT state. */
            if (sock.inode == 0)
                continue;

            /* Unpack encoded local/remote ip strings from socket line. */
            sscanf(packedLocalIP, "%X", &sock.localIP);
            sscanf(packedRemoteIP, "%X", &sock.remoteIP);

            PacketHash hash;
            /* Create packet hash from socket information
             * for future sniffed packet to match against. */
            if (table == "/proc/net/udp")
            {
                hash = PacketHash(sock.localPort);
            }
            else
            {
                hash = PacketHash(sock.localIP, sock.localPort, sock.remoteIP, sock.remotePort);
            }

            mSocketMap[hash] = sock.inode;
        }
    }
}

inode SocketIndex::get(const Packet& pkt)
{

    PacketHash hash(pkt);

    /* If we have recently failed to find the proc net line for the given inode already,
     * don't search for it again. */
    std::unique_lock<std::mutex> lock(mMutex);
    if (mCouldNotFind.count(hash))
    {
        return 0;
    }

    const auto& found = mSocketMap.find(hash);
    if (found != mSocketMap.end())
    {
        return found->second;
    }
    else
    {
        if (pkt.type == PacketType::TCP)
        {
            /* TODO: Handle IPV4 Mapped IPV6 addresses, and/or fully support IPV6 (runelite) */
            /* Just parsing tcp6 table like ipv4 tcp table works ?? (investigate) */
            refresh({"/proc/net/tcp"});
            refresh({"/proc/net/tcp6"});
        }
        else if (pkt.type == PacketType::UDP)
        {
            refresh({"/proc/net/udp"});
        }
        else
        {
            // refresh({"/proc/net/tcp", "/proc/net/udp", "/proc/net/raw"});
            return 0;
        }

        const auto& found = mSocketMap.find(hash);
        if (found != mSocketMap.end())
        {
            return found->second;
        }
        else
        {
            std::cerr << ntmd::logdebug
                      << "Could not find an associated socket inode for the packet: " << pkt
                      << "\n";
            mCouldNotFind[hash] = true;
            return 0;
        }
    }
}

} // namespace ntmd