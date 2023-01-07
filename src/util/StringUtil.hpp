#pragma once

#include <string>
#include <vector>

namespace ntmd::util {

/* Removes whitespace, and newlines from the beginning and end of a string.
 * Does not modify original string, returns a new trimmed string.
 */
std::string trim(const std::string& s);

std::vector<std::string> split(const std::string& s, const char& delimiter = ' ');

/* Modifies string inplace to be all lowercase. Returns a reference to the modified string which
 * will be the same address as the string given as the parameter. */
std::string& strToLower(std::string& s);

/* Takes a string and checks for valid boolean strings such as true/false, yes/no, on/off, and 1/0.
 * Throws an exception if not any of these. */
bool stringToBool(const std::string& s);

/* Determines if a string represents a number. All characters are digits. */
bool isNumber(const std::string& s);

bool containsSemicolon(const std::string& s);

} // namespace ntmd::util