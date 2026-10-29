#pragma once

#include <cctype>
#include <ctime>
#include <string>
#include <vector>

#include "utils/Exceptions.h"

namespace scapms::utils {

inline std::string toLower(const std::string& s)
{
    std::string out = s;
    for (char& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

inline std::string trim(const std::string& s)
{
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

// True if `query` appears anywhere in `text`, ignoring case.
inline bool containsIgnoreCase(const std::string& text, const std::string& query)
{
    return toLower(text).find(toLower(query)) != std::string::npos;
}

inline bool isDigits(const std::string& s)
{
    if (s.empty()) {
        return false;
    }
    for (char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

// Splits on every separator and keeps empty fields ("a,,b" -> "a", "", "b").
inline std::vector<std::string> split(const std::string& line, char sep)
{
    std::vector<std::string> parts;
    std::string current;
    for (char c : line) {
        if (c == sep) {
            parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    parts.push_back(current);
    return parts;
}

// YYYY-MM-DD with month 1-12 and day 1-31.
inline bool isValidDate(const std::string& d)
{
    if (d.size() != 10 || d[4] != '-' || d[7] != '-') {
        return false;
    }
    const std::string year = d.substr(0, 4);
    const std::string month = d.substr(5, 2);
    const std::string day = d.substr(8, 2);
    if (!isDigits(year) || !isDigits(month) || !isDigits(day)) {
        return false;
    }
    const int m = std::stoi(month);
    const int dd = std::stoi(day);
    return m >= 1 && m <= 12 && dd >= 1 && dd <= 31;
}

// HH:MM with 00-23 and 00-59.
inline bool isValidTime(const std::string& t)
{
    if (t.size() != 5 || t[2] != ':') {
        return false;
    }
    const std::string hours = t.substr(0, 2);
    const std::string minutes = t.substr(3, 2);
    if (!isDigits(hours) || !isDigits(minutes)) {
        return false;
    }
    return std::stoi(hours) <= 23 && std::stoi(minutes) <= 59;
}

// Minutes since midnight for a valid HH:MM string.
inline int minutesOfDay(const std::string& t)
{
    return std::stoi(t.substr(0, 2)) * 60 + std::stoi(t.substr(3, 2));
}

inline std::string formatNow(const char* format)
{
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);
    char buffer[32];
    std::strftime(buffer, sizeof buffer, format, local);
    return buffer;
}

inline std::string today() { return formatNow("%Y-%m-%d"); }
inline std::string currentTime() { return formatNow("%H:%M"); }

// Commas would break the CSV files, so text fields may not contain them.
inline void requireNoCommas(const std::string& value, const std::string& field)
{
    if (value.find(',') != std::string::npos) {
        throw ValidationException(field + " cannot contain commas");
    }
}

} // namespace scapms::utils
