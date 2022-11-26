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
    ProcessResolver() = default;
    ~ProcessResolver() = default;

    /* Uses both the socket index and process index to
     * attempt to find a Process associated with the sniffed packet given. */
    std::optional<std::reference_wrapper<const Process>> resolve(const Packet& pkt);

  private:
    SocketIndex mSocketIndex;
    ProcessIndex mProcessIndex;
};

} // namespace ntmd