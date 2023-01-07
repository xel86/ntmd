#pragma once

#include "ProcessIndex.hpp"
#include "SocketIndex.hpp"
#include "net/Packet.hpp"

#include <functional>
#include <optional>

namespace ntmd {

class ProcessResolver
{
  public:
    ProcessResolver(int processIndexCacheSize) : mProcessIndex(processIndexCacheSize) {}
    ~ProcessResolver() = default;

    /* Uses both the socket index and process index to
     * attempt to find a Process associated with the sniffed packet given. */
    const Process& resolve(const Packet& pkt);

  private:
    SocketIndex mSocketIndex;
    ProcessIndex mProcessIndex;

    /* Placeholder process value to return for packets that were failed to be searched for either in
     * the SocketIndex or ProcessIndex. Instead of just ignoring the traffic that was failed to be
     * matched with a process, we will report it back to the user as unknown. */
    const Process mUnknownProcess{"Unknown Traffic", 0};
};

} // namespace ntmd