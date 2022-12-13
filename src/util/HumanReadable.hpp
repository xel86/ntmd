#pragma once

#include <string>

namespace ntmd::util {

/* Represent bytes as a string with a human readable byte postfix.
 * Ex: BytesToHuman(1000) -> "1.0 KB" */
std::string bytesToHuman(uint64_t bytes);

/* Represents bytes over a time period as a string
 * using a human readable byte postfix per second.
 * Ex: BytesToHumanOvertime(10000, 10) -> "1.0 KB/s" */
std::string bytesToHumanOvertime(uint64_t bytes, unsigned seconds);

} // namespace ntmd::util