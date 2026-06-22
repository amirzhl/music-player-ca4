#pragma once

#include <string>
#include <vector>

// Small collection of stateless string/format helpers shared across the project.
// Kept in a dedicated namespace so no mutable global state is introduced.
namespace utils {

// Remove leading/trailing whitespace (spaces, tabs, CR and LF).
std::string trim(const std::string& s);

// Lower-cased copy (ASCII).
std::string toLower(const std::string& s);

// Case-insensitive equality.
bool iequals(const std::string& a, const std::string& b);

// Case-insensitive "contains" check.
bool icontains(const std::string& haystack, const std::string& needle);

// Format a number of seconds as MM:SS (zero padded), e.g. 187 -> "03:07".
std::string formatDuration(int totalSeconds);

// Split a single CSV line into fields. Supports double-quoted fields and
// escaped quotes ("").
std::vector<std::string> splitCsvLine(const std::string& line);

} // namespace utils
