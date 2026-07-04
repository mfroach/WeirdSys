#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>

namespace weirdsys {

struct RegistryValue {
    std::string hive;
    std::string key;
    std::string name;
    std::string value;
    std::string type;
};

struct WmiResult {
    std::string class_name;
    std::vector<std::string> properties;
    std::vector<std::string> values;
};

struct ServiceInfo {
    std::string name;
    std::string display_name;
    std::string state;
    std::string start_type;
    std::string process_id;
    std::string path;
};

struct DriverInfo {
    std::string name;
    std::string base;
    std::string size;
    std::string path;
    std::string version;
};

struct SystemMetric {
    std::string name;
    int value;
    int minimum;
    int maximum;
};

struct NetworkAdapter {
    std::string name;
    std::string mac_address;
    bool ip_configured;
    std::string ip_address;
    std::string subnet_mask;
    std::string default_gateway;
};

struct Hotfix {
    std::string csq;
    std::string kb_id;
    std::string installed_date;
    std::string description;
    std::string installed_by;
};

struct EnvironmentVariable {
    std::string name;
    std::string value;
};

struct CollectedData {
    std::string key;
    std::string value;
};

std::string escapeJson(const std::string& str);

bool valuesMatch(const std::string& baseline, const std::string& current);

std::string evaluateSeverity(const std::string& key,
                            const std::string& baseline,
                            const std::string& current);

}  // namespace weirdsys
