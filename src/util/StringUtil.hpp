#pragma once

#include <string>

namespace ntmd::util {

/* Removes whitespace from the beginning and end of a string.
 * Does not modify original string, returns a new trimmed string.
 */
std::string trim(const std::string& s);

} // namespace ntmd::util