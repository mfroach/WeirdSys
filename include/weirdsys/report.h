#pragma once

#include "types.h"
#include "diff.h"
#include <string>
#include <vector>
#include <memory>

namespace weirdsys {

class CollectorManager {
public:
    CollectorManager();
    virtual ~CollectorManager() = default;
    std::vector<CollectedData> collect();
    std::vector<CollectedData> collectAll();

private:
    std::vector<CollectedData> collectRegistry();
    std::vector<CollectedData> collectWmi();
    std::vector<CollectedData> collectSystem();

    RegistryCollector registry_collector_;
    WmiCollector wmi_collector_;
    SystemCollector system_collector_;
};

enum class ReportFormat { Text, JSON, CSV, HTML };

struct ReportData {
    DiffSummary diff_summary;
    std::string baseline_path;
    std::string current_path;
    std::string report_path;
    bool save_to_file;
};

class ReportGenerator {
public:
    ReportGenerator();
    virtual ~ReportGenerator() = default;

    std::string generate(ReportData data);
    std::string generateBaselineSnapshot(const std::vector<CollectedData>& data,
                                         const std::string& output_path);
    std::string generateJsonReport(ReportData data);

protected:
    std::string generateTextReport(ReportData data);
    std::string generateCsvReport(ReportData data);
    std::string generateHtmlReport(ReportData data);
    std::string formatDriftEntry(const DiffEntry& entry);
    std::string colorizeSeverity(DriftSeverity severity);
    std::string getCurrentTimestamp();

};

}  // namespace weirdsys
