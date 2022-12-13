#include "HumanReadable.hpp"

#include <cstring>

namespace ntmd::util {

const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};

std::string bytesToHuman(uint64_t bytes)
{
    double b = static_cast<double>(bytes);

    int i = 0;
    while (b > 1000)
    {
        b /= 1000;
        i++;
    }

    char str[16];
    sprintf(str, "%.2f %s", b, units[i]);

    return std::string(str);
}

std::string bytesToHumanOvertime(uint64_t bytes, unsigned seconds)
{
    double b = static_cast<double>(bytes) / seconds;

    int i = 0;
    while (b > 1000)
    {
        b /= 1000;
        i++;
    }

    char str[16];
    sprintf(str, "%.*f %s/s", i, b, units[i]);

    return std::string(str);
}

} // namespace ntmd::util