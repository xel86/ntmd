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
    using OptionalProcessRef = std::optional<std::reference_wrapper<const Process>>;

  public:
    ProcessIndex();
    ~ProcessIndex();

    /* Scan and update our process map with socket inodes for every PID folder in /proc.
     * This is CPU intensive. */
    void refresh();

    /* Attempt to find and return a Process from the mProcessMap based on the inode key. */
    OptionalProcessRef get(inode inode);

  private:
    /* Search the /proc directory for a specific socket inode and once found do not search any
     * further. First search through the cached pids, then the entire /proc folder but sorted with
     * the newest processes searched first. */
    OptionalProcessRef search(inode target);

    /* Refresh only the PID folders in the cache, returns a reference to the process if the given
     * inode was found. */
    OptionalProcessRef searchCache(inode target);

    /* Search a pid dir's file descriptor folder (/proc/123/fd) for a specific socket inode.
     * This will also update mProcessMap.
     * If no socket inode target provided, simply ignore the returned OptionalProcessRef.
     */
    OptionalProcessRef processPidDir(const std::string& fdPath, const std::string& pidStr,
                                     const pid_t& pid, inode target = 0);

    /* A process can own multiple socket file descriptors with different inodes, so for quick
     * access of the same process for multiple different socket inodes different inode keys can
     * point to the same process in memory.*/
    std::unordered_map<inode, Process> mProcessMap;

    /* Cache of PIDs who's folder was most recently searched for that contained a new socket file
     * descriptor. This alleviates a lot of CPU cycles for programs that create sockets often,
     * avoiding full /proc refreshs to find new sockets from these cached programs.
     * Discards the least recently used pid once reached max size. */
    LRUArray<pid_t> mLRUCache = LRUArray<pid_t>(5);

    /* For packets and their socket inodes that we cannot find a corresponding process for, add them
     * to a not found list so that we don't continously hammer the CPU trying to find a process that
     * we already know we can't find for every additional packet sniffed. Idealy this list should be
     * empty or very small. */
    std::unordered_map<inode, bool> mCouldNotFind;
    std::mutex mMutex;
};

} // namespace ntmd