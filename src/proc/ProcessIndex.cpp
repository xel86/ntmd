#include "ProcessIndex.hpp"
#include "util/LRUArray.hpp"
#include "util/StringUtil.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>

namespace ntmd {

using inode = uint64_t;

ProcessIndex::ProcessIndex() { refresh(); }

void ProcessIndex::refresh()
{
    for (const auto& dirEntry : std::filesystem::directory_iterator{"/proc"})
    {
        const std::string& dir = dirEntry.path().filename();
        if (!util::isNumber(dir))
            continue;

        pid_t pid = std::stoi(dir);

        /* Do not search PID that are in cache since they will have been searched already. */
        if (mLRUCache.contains(pid))
        {
            continue;
        }

        try
        {

            /* Get name of process through the comm file. */
            std::string comm;
            std::ifstream commFile("/proc/" + dir + "/comm");
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

            /* Find any socket file descriptors the process may own. */
            for (const auto& fd : std::filesystem::directory_iterator{"/proc/" + dir + "/fd"})
            {
                if (!fd.is_symlink())
                    continue;

                std::string link;
                try
                {
                    link = std::filesystem::read_symlink(fd);
                }
                catch (const std::filesystem::filesystem_error& fe)
                {
                    std::cerr
                        << "Tried to read a socket symlink that was deleted after it was found. ("
                        << fd << ")\n";
                }

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
        }
        catch (const std::filesystem::filesystem_error& fe)
        {
            std::cerr << "Tried to read from a process's PID folder that was deleted after it was "
                         "found. ("
                      << "/proc/ " << pid << ")\n";
        }
    }
}

bool ProcessIndex::refreshCached(inode target)
{
    bool found = false;
    for (const pid_t& pid : mLRUCache.iterator())
    {
        if (found)
            break;

        std::string strPid = std::to_string(pid);
        std::string fdPath = "/proc/" + strPid + "/fd";

        /* If the PID in cache no longer exists, skip it. */
        if (!std::filesystem::exists(fdPath))
        {
            /* TODO: Remove from cache? (Add erase method to LRUArray) */
            continue;
        }

        /* Get name of process through the comm file. */
        std::string comm;
        std::ifstream commFile("/proc/" + strPid + "/comm");
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

        for (const auto& fd : std::filesystem::directory_iterator{fdPath})
        {
            if (!fd.is_symlink())
                continue;

            std::string link;
            try
            {
                link = std::filesystem::read_symlink(fd);
            }
            catch (const std::filesystem::filesystem_error& fe)
            {
                std::cerr << "Tried to read a socket symlink that was deleted after it was found. ("
                          << fd << ")\n";
            }

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