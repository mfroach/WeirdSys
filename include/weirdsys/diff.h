#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <map>
#include <fstream>

namespace weirdsys {

enum class DriftSeverity { INFO, WARNING, ERROR, CRITICAL };

struct DiffEntry {
    std::string key;
    std::string baseline_value;
    std::string current_value;
    DriftSeverity severity;
    std::string description;
};

struct DiffSummary {
    std::string timestamp;
    std::string hostname;
    std::string baseline_version;
    std::string current_version;
    int total_entries;
    int matched;
    int drifted;
    std::map<DriftSeverity, int> severity_count;
    std::vector<DiffEntry> entries;
};

class DiffEngine {
public:
    DiffEngine();
    virtual ~DiffEngine() = default;

    DiffSummary compare(const std::string& baseline, const std::string& current);
    DiffSummary compare(const std::map<std::string, std::string>& baseline_map,
                        const std::map<std::string, std::string>& current_map);
    std::map<std::string, std::string> parseBaseline(const std::string& filepath);

private:
    std::map<std::string, std::string> parseSnapshot(const std::string& snapshot);
    std::string formatEntry(const DiffEntry& entry, const std::string& format);
};

}  // namespace weirdsys
