#include "ProcessIndex.hpp"
#include "util/StringUtil.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

namespace ntmd {

ProcessIndex::ProcessIndex() { refresh(); }

void ProcessIndex::refresh()
{
    for (const auto& dirEntry : std::filesystem::directory_iterator{"/proc"})
    {
        const std::string& dir = dirEntry.path().filename();
        if (!util::isNumber(dir))
            continue;

        pid_t pid = std::stoi(dir);

        /* Find any socket file descriptors the process may own. */
        for (const auto& fd : std::filesystem::directory_iterator{"/proc/" + dir + "/fd"})
        {
            if (!fd.is_symlink())
                continue;

            const std::string& link = std::filesystem::read_symlink(fd);
            inode inode;
            if (link.rfind("socket:", 0) == 0)
            {
                /* socket:[12345]  -> 12345 */
                inode = std::stoi(link.substr(8, link.size()));

                std::ifstream comm("/proc/" + dir + "/comm");
                Process process;
                std::getline(comm, process.comm);

                mProcessMap[inode] = process;
            }
        }
    }
}

ProcessIndex::~ProcessIndex() {}

} // namespace ntmd