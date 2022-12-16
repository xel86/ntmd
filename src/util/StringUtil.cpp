#include "StringUtil.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace ntmd::util {

std::string trim(const std::string& s)
{
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start))
    {
        start++;
    }

    auto end = s.end();
    do
    {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));

    return std::string(start, end + 1);
}

std::string& strToLower(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

bool stringToBool(const std::string& s)
{
    if (s == "true" || s == "yes" || s == "on" || s == "1")
        return true;
    else if (s == "false" || s == "no" || s == "off" || s == "0")
        return false;
    else
        throw std::invalid_argument(
            "String value does not match a valid boolean string representation.");
}

bool isNumber(const std::string& s)
{
    if (s.empty())
        return false;

    for (const char& c : s)
    {
        if (!std::isdigit(c))
            return false;
    }

    return true;
}

bool containsSemicolon(const std::string& s)
{
    if (s.empty())
        return false;

    for (const char& c : s)
    {
        if (c == ';')
            return true;
    }

    return false;
}

} // namespace ntmd::util