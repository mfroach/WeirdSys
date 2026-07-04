#include "weirdsys/diff.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <memory>
#include <ctime>
#include <chrono>

namespace weirdsys {

DiffEngine::DiffEngine() {}

std::map<std::string, std::string> DiffEngine::parseBaseline(
    const std::string& filepath) {
    std::map<std::string, std::string> snapshot;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return snapshot;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find(':');
        if (pos == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        if (!key.empty()) {
            snapshot[key] = value;
        }
    }

    return snapshot;
}

std::map<std::string, std::string> DiffEngine::parseSnapshot(const std::string& snapshot) {
    std::map<std::string, std::string> data;
    std::istringstream iss(snapshot);
    std::string line;

    while (std::getline(iss, line)) {
        size_t pos = line.find(':');
        if (pos == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        if (!key.empty()) {
            data[key] = value;
        }
    }

    return data;
}

DiffSummary DiffEngine::compare(const std::string& baseline,
                                const std::string& current) {
    std::map<std::string, std::string> baseline_map = parseBaseline(baseline);
    std::map<std::string, std::string> current_map = parseSnapshot(current);
    return compare(baseline_map, current_map);
}

DiffSummary DiffEngine::compare(
    const std::map<std::string, std::string>& baseline_map,
    const std::map<std::string, std::string>& current_map) {

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now{};
    localtime_r(&time_t_now, &tm_now);

    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
    std::string timestamp_str = oss.str();

    DiffSummary summary;
    summary.timestamp = timestamp_str;
    summary.hostname = "unknown";
    summary.baseline_version = "1.0";
    summary.current_version = "1.0";

    std::map<std::string, std::string> baseline = baseline_map;

    std::vector<DiffEntry> entries;
    std::map<DriftSeverity, int> severity_count;

    for (const auto& [key, current_value] : current_map) {
        std::string baseline_value = baseline.count(key) ? baseline.at(key) : "";
        std::string sev_str = evaluateSeverity(key, baseline_value, current_value);

        // Convert string back to DriftSeverity
        DriftSeverity sev = static_cast<DriftSeverity>(sev_str == "CRITICAL" ? 3 :
                                sev_str == "ERROR" ? 2 :
                                sev_str == "WARNING" ? 1 : 0);

        DiffEntry entry;
        entry.key = key;
        entry.baseline_value = baseline_value;
        entry.current_value = current_value;
        entry.severity = sev;
        entry.description = "";

        entries.push_back(entry);
        severity_count[sev]++;
    }

    int total = static_cast<int>(entries.size());
    int drifted = 0;
    for (const auto& e : entries) {
        if (e.baseline_value.empty() || !valuesMatch(e.baseline_value, e.current_value)) {
            drifted++;
        }
    }

    summary.total_entries = total;
    summary.matched = total - drifted;
    summary.drifted = drifted;
    summary.severity_count = severity_count;
    summary.entries = entries;

    return summary;
}

}  // namespace weirdsys
