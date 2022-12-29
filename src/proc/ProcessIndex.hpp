#pragma once

#include "util/LRUArray.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

namespace ntmd {

struct Process
{
    std::string comm;
    pid_t pid;
};

/* Index of all processes and their associated socket file descriptors in the /proc directory. */
class ProcessIndex
{
    using inode = uint64_t;

  public:
    ProcessIndex();
    ~ProcessIndex();

    /* Scan and update our process map with socket inodes for every PID folder in /proc.
     * This is CPU intensive. */
    void refresh();

    /* Refresh only the PID folders in the cache, returns true if the given inode was found. */
    bool refreshCached(inode target);

    /* Attempt to find and return a Process from the mProcessMap based on the inode key. */
    std::optional<std::reference_wrapper<const Process>> get(inode inode);

  private:
    /* A process can own multiple socket file descriptors with different inodes, so for quick access
     * of the same process for multiple different socket inodes different inode keys can point to
     * the same process in memory.*/
    std::unordered_map<inode, Process> mProcessMap;

    /* Cache of PIDs who's folder was most recently searched for that contained a new socket file
     * descriptor. This alleviates a lot of CPU cycles for programs that create sockets often,
     * avoiding full /proc refreshs to find new sockets from these cached programs.
     * Discards the least recently used pid once reached max size. */
    LRUArray<pid_t> mLRUCache = LRUArray<pid_t>(3);

    /* For packets and their socket inodes that we cannot find a corresponding process for, add them
     * to a not found list so that we don't continously hammer the CPU trying to find a process that
     * we already know we can't find for every additional packet sniffed. Idealy this list should be
     * empty or very small. */
    std::unordered_map<inode, bool> mCouldNotFind;
    std::mutex mMutex;
};

} // namespace ntmd