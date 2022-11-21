#pragma once

#include <filesystem>

namespace ntmd::util {

/* Attempts to get a home directory from XDG or HOME environment variables.
 * Returns an empty path if home directory couldn't be found.
 */
std::filesystem::path getHomeDirectory();

} // namespace ntmd::util