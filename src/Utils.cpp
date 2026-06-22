#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace utils {

std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    const std::size_t begin = s.find_first_not_of(ws);
    if (begin == std::string::npos) {
        return "";
    }
    const std::size_t end = s.find_last_not_of(ws);
    return s.substr(begin, end - begin + 1);
}

std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool iequals(const std::string& a, const std::string& b) {
    return toLower(a) == toLower(b);
}

bool icontains(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) {
        return true;
    }
    return toLower(haystack).find(toLower(needle)) != std::string::npos;
}

std::string formatDuration(int totalSeconds) {
    if (totalSeconds < 0) {
        totalSeconds = 0;
    }
    const int minutes = totalSeconds / 60;
    const int seconds = totalSeconds % 60;
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << minutes << ":"
        << std::setw(2) << std::setfill('0') << seconds;
    return oss.str();
}

std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool inQuotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i; // skip the escaped quote
                } else {
                    inQuotes = false;
                }
            } else {
                field += c;
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                fields.push_back(field);
                field.clear();
            } else {
                field += c;
            }
        }
    }
    fields.push_back(field);
    return fields;
}

} // namespace utils
