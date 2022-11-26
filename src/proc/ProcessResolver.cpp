#include "ProcessResolver.hpp"

#include "ProcessIndex.hpp"
#include "net/Packet.hpp"
#include "proc/ProcessIndex.hpp"
#include "proc/SocketIndex.hpp"

#include <functional>
#include <optional>

namespace ntmd {

std::optional<std::reference_wrapper<const Process>> ProcessResolver::resolve(const Packet& pkt)
{
    uint64_t inode = mSocketIndex.get(pkt);
    if (inode == 0)
        return std::nullopt;

    auto process = mProcessIndex.get(inode);

    if (process.has_value())
    {
        return process;
    }
    else
    {
        return std::nullopt;
    }
}

} // namespace ntmd