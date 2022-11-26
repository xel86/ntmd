#pragma once

#include <cstdint>
#include <memory>
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

  private:
    /* A process can own multiple socket file descriptors with different inodes, so for quick access
     * of the same process for multiple different socket inodes different inode keys can point to
     * the same process in memory.*/
    std::unordered_map<inode, Process> mProcessMap;
};

} // namespace ntmd