#include "weirdsys/report.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <memory>

namespace weirdsys {

ReportGenerator::ReportGenerator() {}

ReportGenerator::~ReportGenerator() = default;

std::string ReportGenerator::generate(ReportData data) {
    return generateTextReport(data);
}

std::string ReportGenerator::generateBaselineSnapshot(
    const std::vector<CollectedData>& data, const std::string& output_path) {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"generated_at\": \"" << getCurrentTimestamp() << "\",\n";
    oss << "  \"hostname\": \"unknown\",\n";
    oss << "  \"baseline_version\": \"1.0\",\n";
    oss << "  \"entries\": [\n";
    bool first = true;
    for (const auto& entry : data) {
        if (!first) oss << ",\n";
        oss << "    {\"key\": \"" << escapeJson(entry.key) << "\",\n";
        oss << "     \"value\": \"" << escapeJson(entry.value) << "\"\n";
        oss << "    }";
        first = false;
    }
    oss << "\n  ]\n";
    oss << "}\n";
    return oss.str();
}

std::string ReportGenerator::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_s(&tm_now, &time_t_now);
    std::string timestamp_str = std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
    return timestamp_str;
}

std::string ReportGenerator::generateTextReport(ReportData data) {
    std::ostringstream oss;

    oss << "========================================\n";
    oss << "     WEIRD SYS DIAGNOSTIC REPORT\n";
    oss << "========================================\n\n";

    oss << "Baseline:    " << data.baseline_path << "\n";
    oss << "Current:     " << data.current_path << "\n\n";

    oss << "Diff Summary\n";
    oss << "------------\n";
    oss << "  Total entries:  " << data.diff_summary.total_entries << "\n";
    oss << "  Matched:        " << data.diff_summary.matched << "\n";
    oss << "  Drifted:        " << data.diff_summary.drifted << "\n";

    oss << "\n  Severity breakdown:\n";
    for (const auto& [severity, count] : data.diff_summary.severity_count) {
        oss << "    " << severity << ": " << count << "\n";
    }

    oss << "\nDrift Details\n";
    oss << "-------------\n";

    if (data.diff_summary.entries.empty()) {
        oss << "  No differences detected.\n";
    } else {
        for (const auto& entry : data.diff_summary.entries) {
            oss << "\n" << formatDriftEntry(entry) << "\n";
        }
    }

    oss << "\n========================================\n";

    return oss.str();
}

std::string ReportGenerator::generateJsonReport(ReportData data) {
    std::ostringstream oss;
    oss << "{\n  \"timestamp\": \"" << data.diff_summary.timestamp << "\",\n";
    oss << "  \"hostname\": \"" << data.diff_summary.hostname << "\",\n";
    oss << "  \"baseline_version\": \"" << data.diff_summary.baseline_version << "\",\n";
    oss << "  \"current_version\": \"" << data.diff_summary.current_version << "\",\n";
    oss << "  \"total_entries\": " << data.diff_summary.total_entries << ",\n";
    oss << "  \"matched\": " << data.diff_summary.matched << ",\n";
    oss << "  \"drifted\": " << data.diff_summary.drifted << ",\n";
    oss << "  \"severity_breakdown\": {\n";
    bool first = true;
    for (const auto& [severity, count] : data.diff_summary.severity_count) {
        if (!first) oss << ",\n";
        oss << "    \"" << severity << "\": " << count;
        first = false;
    }
    oss << "\n  },\n";
    oss << "  \"entries\": [\n";

    first = true;
    for (const auto& entry : data.diff_summary.entries) {
        if (!first) oss << ",\n";
        oss << "    {\n";
        oss << "      \"key\": \"" << entry.key << "\",\n";
        oss << "      \"baseline_value\": \"" << escapeJson(entry.baseline_value) << "\",\n";
        oss << "      \"current_value\": \"" << escapeJson(entry.current_value) << "\",\n";
        oss << "      \"severity\": \"" << escapeJson(entry.severity) << "\",\n";
        if (!entry.description.empty()) {
            oss << "      \"description\": \"" << escapeJson(entry.description) << "\"\n";
        } else {
            oss << "      \"description\": \"\"\n";
        }
        oss << "    }";
        first = false;
    }
    oss << "\n  ]\n";
    oss << "}\n";

    return oss.str();
}

std::string ReportGenerator::generateCsvReport(ReportData data) {
    return "";
}

std::string ReportGenerator::generateHtmlReport(ReportData data) {
    return "";
}

std::string ReportGenerator::formatDriftEntry(const DiffEntry& entry) {
    std::ostringstream oss;
    oss << "  Key:        " << entry.key << "\n";
    oss << "  Baseline:   " << entry.baseline_value << "\n";
    oss << "  Current:    " << entry.current_value << "\n";

    std::string severity_str;
    switch (entry.severity) {
        case DriftSeverity::CRITICAL:
            severity_str = "CRITICAL";
            break;
        case DriftSeverity::ERROR:
            severity_str = "ERROR";
            break;
        case DriftSeverity::WARNING:
            severity_str = "WARNING";
            break;
        default:
            severity_str = "INFO";
    }
    oss << "  Severity:   " << severity_str << "\n";

    if (!entry.description.empty()) {
        oss << "  Note:       " << entry.description << "\n";
    }

    return oss.str();
}

std::string ReportGenerator::colorizeSeverity(DriftSeverity severity) {
    switch (severity) {
        case DriftSeverity::CRITICAL:
            return "\033[31mCRITICAL\033[0m";
        case DriftSeverity::ERROR:
            return "\033[31mERROR\033[0m";
        case DriftSeverity::WARNING:
            return "\033[33mWARNING\033[0m";
        default:
            return "\033[32mINFO\033[0m";
    }
}

std::string ReportGenerator::escapeJson(const std::string& str) {
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

bool ReportGenerator::saveToFile(ReportData data) {
    return false;
}

}  // namespace weirdsys
