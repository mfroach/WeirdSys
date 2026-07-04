#include "weirdsys/types.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace weirdsys {

std::string normalizeValue(const std::string& value) {
    if (value.empty())
        return "_empty";

    std::string result;
    result.reserve(value.length());

    for (char c : value) {
        if (c >= 'a' && c <= 'z')
            result += static_cast<char>(c - 'a' + 'A');
        else if (c >= '0' && c <= '9')
            result += c;
        else
            result += '_';
    }

    return result.empty() ? "_empty" : result;
}

std::string escapeJson(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
            case '"':  oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default: oss << c;
        }
    }
    return oss.str();
}

bool valuesMatch(const std::string& baseline, const std::string& current) {
    return normalizeValue(baseline) == normalizeValue(current);
}

std::string evaluateSeverity(const std::string& key,
                            const std::string& baseline,
                            const std::string& current) {
    const std::string criticalKeys[] = {
        "HKLM\\SYSTEM\\CurrentControlSet\\Services",
        "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
        "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        "HKLM\\SOFTWARE\\Policies"
    };

    bool isCritical = false;
    for (const auto& ck : criticalKeys) {
        if (key.find(ck) != std::string::npos) {
            isCritical = true;
            break;
        }
    }

    if (isCritical && !valuesMatch(baseline, current))
        return "CRITICAL";
    if (isCritical)
        return "ERROR";
    if (!valuesMatch(baseline, current))
        return "WARNING";

    return "INFO";
}

}  // namespace weirdsys
