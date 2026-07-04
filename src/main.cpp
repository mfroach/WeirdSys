#include <iostream>
#include <fstream>
#include <filesystem>
#include <memory>
#include <sstream>
#include <algorithm>

#include "weirdsys/collector.h"
#include "weirdsys/diff.h"
#include "weirdsys/report.h"

namespace fs = std::filesystem;

class DiagnosticRunner {
public:
    DiagnosticRunner()
        : collector_(std::make_unique<weirdsys::CollectorManager>())
        , diff_engine_()
        , report_gen_()
    {}

    int run(const std::string& baseline_path = "",
            bool save_baseline = false,
            const std::string& output_path = "") {

        // Step 1: Collect current system state
        std::cout << "=== Collecting System Information ===" << std::endl;
        auto current_data = collector_->collectAll();

        auto current_snapshot = report_gen_.generateBaselineSnapshot(
            current_data, "current_snapshot.json");

        // Step 2: Compare against baseline if provided
        if (!baseline_path.empty()) {
            std::cout << "\n=== Comparing Against Baseline ===" << std::endl;
            std::cout << "Baseline file: " << baseline_path << std::endl;

            // Parse baseline file
            auto baseline_map = diff_engine_.parseBaseline(baseline_path);

            // Build current snapshot as JSON
            std::ostringstream current_json;
            current_json << current_snapshot;
            std::string current_str = current_json.str();

            // Build baseline as JSON
            std::ostringstream baseline_json;
            for (const auto& [key, value] : baseline_map) {
                baseline_json << "\"" << key << "\": \"" << value << "\",\n";
            }
            std::string baseline_str = baseline_json.str();

            // Compare
            auto diff_summary = diff_engine_.compare(baseline_str, current_str);

            auto report = report_gen_.generate(
                weirdsys::ReportData{diff_summary, baseline_path, "", output_path, true});
            std::cout << "\n" << report << std::endl;

            if (!output_path.empty()) {
                saveToFile(diff_summary, output_path);
            }

        } else {
            std::cout << "\n--- No baseline provided. Showing current state ---" << std::endl;
            std::cout << "\n" << current_snapshot << std::endl;
        }

        // Step 3: Save current state as baseline if requested
        if (save_baseline) {
            std::string baseline_name = "baseline_" +
                fs::path(output_path).filename().string() +
                ".json";
            auto baseline_path_str = "weirdsys_baseline/" + baseline_name;
            fs::create_directories("weirdsys_baseline");
            std::ofstream baseline_file(baseline_path_str);
            baseline_file << current_snapshot << std::endl;
            std::cout << "\nCurrent state saved as baseline: " << baseline_path_str << std::endl;
        }

        return 0;
    }

private:
    static std::vector<weirdsys::CollectedData> extractWmiDataFromResult(const std::vector<weirdsys::CollectedData>& data) {
        std::vector<weirdsys::CollectedData> extracted;
        
        // Extract WMI data from WmiResult - placeholder for future WMI result processing
        // This will be populated when we have the WmiResult data from queries
        return data;
    }

    std::vector<weirdsys::CollectedData> collectAll() {
        return collector_->collectAll();
    }

    void saveToFile(const weirdsys::DiffSummary& summary,
                    const std::string& path) {
        std::ofstream file(path);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file for writing: " << path << std::endl;
            return;
        }

        file << report_gen_.generateJsonReport(
            weirdsys::ReportData{summary, "", "", path, false});
        std::cout << "Report saved to: " << path << std::endl;
    }

    std::unique_ptr<weirdsys::CollectorManager> collector_;
    weirdsys::DiffEngine diff_engine_;
    weirdsys::ReportGenerator report_gen_;
};

}  // namespace weirdsys

int main(int argc, char* argv[]) {
    DiagnosticRunner runner;

    std::string baseline_path = "";
    bool save_baseline = false;
    std::string output_path = "";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--baseline" && i + 1 < argc) {
            baseline_path = argv[++i];
        } else if (arg == "--save-baseline") {
            save_baseline = true;
        } else if (arg == "--output" && i + 1 < argc) {
            output_path = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: weirdsys [--baseline <file>] [--save-baseline] [--output <file>]\n"
                      << "\nOptions:\n"
                      << "  --baseline <file>    Path to baseline snapshot file\n"
                      << "  --save-baseline      Save current state as baseline\n"
                      << "  --output <file>      Output report file path\n"
                      << "  --help, -h           Show this help message\n" << std::endl;
            return 0;
        }
    }

    return runner.run(baseline_path, save_baseline, output_path);
}
