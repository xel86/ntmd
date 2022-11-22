#pragma once

#include <string>

namespace ntmd::util {

/* Removes whitespace from the beginning and end of a string.
 * Does not modify original string, returns a new trimmed string.
 */
std::string trim(const std::string& s);

/* Modifies string inplace to be all lowercase. Returns a reference to the modified string which
 * will be the same address as the string given as the parameter. */
std::string& strToLower(std::string& s);

/* Takes a string and checks for valid boolean strings such as true/false, yes/no, on/off, and 1/0.
 * Throws an exception if not any of these. */
bool stringToBool(const std::string& s);

} // namespace ntmd::util