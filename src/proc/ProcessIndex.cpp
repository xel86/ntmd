#include "ProcessIndex.hpp"
#include "util/LRUArray.hpp"
#include "util/StringUtil.hpp"

#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <unistd.h>

namespace ntmd {

using inode = uint64_t;

ProcessIndex::ProcessIndex() { refresh(); }

void ProcessIndex::refresh()
{
    /* Throughout the project I've tried to stick with using mostly modern c++ abstractions that are
     * nearly zero-cost, but unfortunately in the situation of iterating over directories often
     * std::filesystem::directory_iterator is painfully slow compared to using DIR & dirent. */

    DIR* procDir = opendir("/proc");
    if (procDir == nullptr)
    {
        std::cerr << "Failed to open the /proc directory, error: " << strerror(errno)
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
        const pid_t pid = std::stoi(pidStr);

        /* Do not search PIDs that are in cache since they will have been searched already. */
        if (mLRUCache.contains(pid))
        {
            continue;
        }

        std::string comm;
        std::ifstream commFile("/proc/" + pidStr + "/comm");
        if (!commFile.is_open())
        {
            std::cerr << "Error opening comm file for PID " << pid
                      << ". Process could have been deleted while processing it.\n";
            break;
        }
        else
        {
            std::getline(commFile, comm);
        }

        const std::string fdPath = "/proc/" + pidStr + "/fd";
        DIR* fdDir = opendir(fdPath.c_str());
        if (fdDir == nullptr)
        {
            std::cerr << "Tried to read from a process's file descriptor folder that was deleted "
                         "after it was "
                         "found. ("
                      << "/proc/ " << pidStr << ")\n";
            continue;
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

                mProcessMap[inode] = process;
            }
        }

        closedir(fdDir);
    }

    closedir(procDir);
}

bool ProcessIndex::refreshCached(inode target)
{
    bool found = false;
    for (const pid_t& pid : mLRUCache.iterator())
    {
        if (found)
            break;

        const std::string pidStr = std::to_string(pid);
        const std::string fdPath = "/proc/" + pidStr + "/fd";

        /* If the PID in cache no longer exists, skip it. */
        if (!std::filesystem::exists(fdPath))
        {
            mLRUCache.erase(pid);
            continue;
        }

        std::string comm;
        std::ifstream commFile("/proc/" + pidStr + "/comm");
        if (!commFile.is_open())
        {
            std::cerr << "Error opening comm file for PID " << pid
                      << ". Process could have been deleted while processing it.\n";
            break;
        }
        else
        {
            std::getline(commFile, comm);
        }

        DIR* fdDir = opendir(fdPath.c_str());
        if (fdDir == nullptr)
        {
            std::cerr << "Tried to read from a process's file descriptor folder that was deleted "
                         "after it was "
                         "found. ("
                      << "/proc/ " << pidStr << ")\n";
            continue;
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

                if (inode == target)
                {
                    std::cerr << "Successful cache hit for process " << process.comm << " inode "
                              << target << "\n";
                    found = true;
                }

                mProcessMap[inode] = process;
            }
        }

        closedir(fdDir);
    }

    return found;
}

std::optional<std::reference_wrapper<const Process>> ProcessIndex::get(inode inode)
{
    const auto& found = mProcessMap.find(inode);
    if (found != mProcessMap.end())
    {
        return found->second;
    }
    else
    {
        /* Attempt to find new socket in the most recently used cache, if we didn't get a hit then
         * do a full refresh */
        bool hit = refreshCached(inode);

        if (!hit)
            refresh();

        const auto& found = mProcessMap.find(inode);
        if (found != mProcessMap.end())
        {
            const Process& process = found->second;
            if (!hit)
                std::cerr << "Succesfully refreshed and found new process: " << process.comm
                          << " with inode: " << inode << "\n";

            mLRUCache.update(process.pid);

            return process;
        }
        else
        {
            std::cerr << "Could not find a process associated with that inode[" << inode << "].\n";
            return std::nullopt;
        }
    }
}

ProcessIndex::~ProcessIndex() {}

} // namespace ntmd