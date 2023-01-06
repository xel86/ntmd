#include "ProcessIndex.hpp"
#include "Daemon.hpp"
#include "util/LRUArray.hpp"
#include "util/StringUtil.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <thread>
#include <unistd.h>

namespace ntmd {

using inode = uint64_t;
using OptionalProcessRef = std::optional<std::reference_wrapper<const Process>>;

ProcessIndex::ProcessIndex(int cacheSize) : mLRUCache(cacheSize)
{
    refresh();

    /* Clear out list of inode's that we have failed to find a process for in the past every 60
     * seconds to avoid re-used inodes from being skipped. */
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

void ProcessIndex::refresh()
{
    /* Throughout the project I've tried to stick with using mostly modern c++ abstractions that are
     * nearly zero-cost, but unfortunately in the situation of iterating over directories often
     * std::filesystem::directory_iterator is painfully slow compared to using DIR & dirent. */

    /* Clear the process map before doing a full scan to prevent old processes from taking up
     * memory, this is performance heavy. We will not clear the mLRUCache because its not too big of
     * a deal for those values to be expired and replaced naturally overtime than to prevent the big
     * performance benefits and clear now. */
    // TODO: We aren't clearing this anymore after a refactor, think of a solution.
    mProcessMap.clear();

    DIR* procDir = opendir("/proc");
    if (procDir == nullptr)
    {
        std::cerr << ntmd::logerror
                  << "Failed to open the /proc directory, error: " << strerror(errno)
                  << ". Cannot proceed, exiting.";
        std::exit(1);
    }

    dirent* procEntry;
    while ((procEntry = readdir(procDir)))
    {
        /* Check if the entry in proc represents a process directory. */
        if (procEntry->d_type != DT_DIR || !util::isNumber(procEntry->d_name))
            continue;

        const std::string& pidStr = procEntry->d_name;
        const std::string fdPath = "/proc/" + pidStr + "/fd";
        const pid_t pid = std::stoi(pidStr);

        /* Do not search PIDs that are in cache since they will have been searched already. */
        if (mLRUCache.contains(pid))
        {
            continue;
        }

        processPidDir(fdPath, pidStr, pid);
    }

    closedir(procDir);
}

OptionalProcessRef ProcessIndex::search(inode target)
{
    OptionalProcessRef foundProcess;

    /* First search the pid's in the cache, and update/remove values inside the cache.
     * If the cache finds the new socket and its associated process, return it. */
    foundProcess = searchCache(target);
    if (foundProcess.has_value())
    {
        return foundProcess;
    }

    /* If the new socket doesn't belong to a cached pid, do a search of the entire pid directory but
     * sorted with the most recently spawned processes searched first (larger pid).
     * To do this we first traverse the entire proc directory to sort the pid's, then scan the fd
     * folder for each pid folder. */
    DIR* procDir = opendir("/proc");
    if (procDir == nullptr)
    {
        std::cerr << ntmd::logerror
                  << "Failed to open the /proc directory, error: " << strerror(errno)
                  << ". Cannot proceed, exiting.";
        std::exit(1);
    }

    std::set<pid_t, std::greater<pid_t>> sortedProcDir;

    dirent* procEntry;
    while ((procEntry = readdir(procDir)))
    {
        /* Check if the entry in proc represents a process directory. */
        if (procEntry->d_type != DT_DIR || !util::isNumber(procEntry->d_name))
            continue;

        const std::string& pidStr = procEntry->d_name;
        const pid_t pid = std::stoi(pidStr);

        /* Do not search PIDs that are in cache since they will have been searched already. */
        if (mLRUCache.contains(pid))
        {
            continue;
        }

        sortedProcDir.insert(pid);
    }
    closedir(procDir);

    /* Search the process folders in proc with the most recently spawned processes being searched
     * first, if we find the socket inode target do not search anymore and return the process it
     * belongs to. */
    for (const pid_t& pid : sortedProcDir)
    {
        const std::string pidStr = std::to_string(pid);
        const std::string fdPath = "/proc/" + pidStr + "/fd";

        foundProcess = processPidDir(fdPath, pidStr, pid, target);

        if (foundProcess.has_value())
        {
            const Process& ref = foundProcess->get();
            std::cerr << ntmd::logdebug
                      << "Successful sorted search and found new process: " << ref.comm
                      << " (pid: " << ref.pid << ") with inode: " << target << "\n";
            return foundProcess;
        }
    }

    return foundProcess;
}

OptionalProcessRef ProcessIndex::searchCache(inode target)
{
    OptionalProcessRef foundProcess;
    std::vector<pid_t> expired;

    for (const pid_t& pid : mLRUCache.iterator())
    {
        const std::string pidStr = std::to_string(pid);
        const std::string fdPath = "/proc/" + pidStr + "/fd";

        /* If the PID in cache no longer exists, skip it. */
        if (!std::filesystem::exists(fdPath))
        {
            expired.push_back(pid);
            continue;
        }

        foundProcess = processPidDir(fdPath, pidStr, pid, target);

        if (foundProcess.has_value())
            return foundProcess;
    }

    /* Erase expired pid's in cache outside of previous loop
     * to ensure the iterator doesn't become invalid. */
    for (const pid_t& pid : expired)
    {
        mLRUCache.erase(pid);
    }

    return foundProcess;
}

OptionalProcessRef ProcessIndex::processPidDir(const std::string& fdPath, const std::string& pidStr,
                                               const pid_t& pid, inode target)
{
    OptionalProcessRef found;

    std::string comm;
    std::ifstream commFile("/proc/" + pidStr + "/comm");
    if (!commFile.is_open())
    {
        std::cerr << ntmd::logdebug << "Error opening comm file for PID " << pid
                  << ". Process could have been deleted while processing it.\n";
        return std::nullopt;
    }
    else
    {
        std::getline(commFile, comm);
    }

    DIR* fdDir = opendir(fdPath.c_str());
    if (fdDir == nullptr)
    {
        std::cerr << ntmd::logdebug
                  << "Tried to read from a process's file descriptor folder that was deleted "
                     "after it was "
                     "found. ("
                  << "/proc/ " << pidStr << ")\n";

        return std::nullopt;
    }

    /* Find any socket file descriptors the process may own. */
    dirent* fdEntry;
    while ((fdEntry = readdir(fdDir)))
    {
        /* Socket file desciptors are always symbolic links */
        if (fdEntry->d_type != DT_LNK)
            continue;

        const std::string linkPath = fdPath + "/" + fdEntry->d_name;
        char linkRaw[80];

        /* Skip if file desciptor was deleted before we were able to read it. */
        if (readlink(linkPath.c_str(), linkRaw, 79) < 0)
        {
            continue;
        }
        const std::string& link = linkRaw;

        inode inode;
        if (link.rfind("socket:", 0) == 0)
        {
            /* socket:[12345]  -> 12345 */
            inode = std::stoi(link.substr(8, link.size()));

            Process process;

            process.pid = pid;
            process.comm = comm;

            const auto& it = mProcessMap.insert_or_assign(inode, process).first;
            if (inode != 0 && inode == target)
            {
                const Process& ref = it->second;
                found = ref;
            }
        }
    }

    closedir(fdDir);

    return found;
}

OptionalProcessRef ProcessIndex::get(inode inode)
{
    /* If we have recently failed to find the process for the given inode already,
     * don't search for it again. */
    std::unique_lock<std::mutex> lock(mMutex);
    if (mCouldNotFind.count(inode))
    {
        return std::nullopt;
    }

    const auto& found = mProcessMap.find(inode);
    if (found != mProcessMap.end())
    {
        return found->second;
    }
    else
    {
        /* Attempt to search for the socket inode and the corresponding process it belongs too.
         * This first searches the mLRUCache, then searches each individual pid proc folder starting
         * with the newest processes first. */
        OptionalProcessRef found = search(inode);

        if (found.has_value())
        {
            const Process& process = found->get();

            mLRUCache.update(process.pid);

            return process;
        }
        else
        {
            std::cerr << ntmd::logdebug << "Could not find a process associated with the inode["
                      << inode << "] found in the SocketIndex.\n";
            mCouldNotFind[inode] = true;
            return std::nullopt;
        }
    }
}

ProcessIndex::~ProcessIndex() {}

} // namespace ntmd