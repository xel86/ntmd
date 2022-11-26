#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

namespace ntmd {

struct Process
{
    std::string comm;
};

/* Index of all processes and their associated socket file descriptors in the /proc directory. */
class ProcessIndex
{
    using inode = uint16_t;

  public:
    ProcessIndex();
    ~ProcessIndex();

    void refresh();

    /* Attempt to find and return a Process from the mProcessMap based on the inode key. */
    std::optional<std::reference_wrapper<const Process>> get(inode inode);

  private:
    /* A process can own multiple socket file descriptors with different inodes, so for quick access
     * of the same process for multiple different socket inodes different inode keys can point to
     * the same process in memory.*/
    std::unordered_map<inode, Process> mProcessMap;
};

} // namespace ntmd