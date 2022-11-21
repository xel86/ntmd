#include "FilesystemUtil.hpp"

#include <filesystem>
#include <iostream>

namespace ntmd::util {

std::filesystem::path getHomeDirectory()
{
    std::filesystem::path dir;
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"))
    {
        dir = xdg;
    }
    else if (const char* home = std::getenv("HOME"))
    {
        dir = home;
    }

    return dir;
}

} // namespace ntmd::util