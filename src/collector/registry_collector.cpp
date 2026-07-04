#include "weirdsys/collector.h"
#include <algorithm>
#include <windows.h>

namespace weirdsys {

RegistryCollector::RegistryCollector() {}

RegistryCollector::~RegistryCollector() = default;

std::vector<CollectedData> RegistryCollector::collect() {
    std::vector<CollectedData> data;

    data.insert(data.end(), collectControl().begin(), collectControl().end());
    data.insert(data.end(), collectServices().begin(), collectServices().end());
    data.insert(data.end(), collectPolicies().begin(), collectPolicies().end());
    data.insert(data.end(), collectStartupEntries().begin(), collectStartupEntries().end());
    data.insert(data.end(), collectEnvironmentRegistry().begin(), collectEnvironmentRegistry().end());

    return data;
}

std::vector<RegistryValue> RegistryCollector::collectControl() {
    std::vector<RegistryValue> values;

    auto control = collectRegistryKey("HKEY_LOCAL_MACHINE",
        "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control");
    for (auto& v : control) {
        if (v.name.empty() || v.name[0] == ' ') continue;
        values.push_back(v);
    }

    return values;
}

std::vector<RegistryValue> RegistryCollector::collectServices() {
    std::vector<RegistryValue> values;

    auto services = collectRegistryKey("HKEY_LOCAL_MACHINE",
        "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services");
    for (auto& v : services) {
        if (v.name.empty() || v.name[0] == ' ') continue;
        values.push_back(v);
    }

    return values;
}

std::vector<RegistryValue> RegistryCollector::collectPolicies() {
    std::vector<RegistryValue> values;

    auto policiesHkLm = collectRegistryKey("HKEY_LOCAL_MACHINE",
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies");
    for (auto& v : policiesHkLm) {
        if (v.name.empty() || v.name[0] == ' ') continue;
        values.push_back(v);
    }

    auto policiesHkCu = collectRegistryKey("HKEY_CURRENT_USER",
        "HKEY_CURRENT_USER\\SOFTWARE\\Policies");
    for (auto& v : policiesHkCu) {
        if (v.name.empty() || v.name[0] == ' ') continue;
        values.push_back(v);
    }

    return values;
}

std::vector<RegistryValue> RegistryCollector::collectRegistryKey(
    const std::string& hive, const std::string& key) {
    std::vector<RegistryValue> values;

    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, key.c_str(), 0,
        KEY_READ | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hKey);

    if (result != ERROR_SUCCESS || hKey == nullptr) {
        return values;
    }

    DWORD subkeyCount = 0;
    RegEnumKeyExA(hKey, 0, nullptr, nullptr, nullptr, nullptr, nullptr,
        &subkeyCount);

    for (DWORD i = 0; i < subkeyCount; ++i) {
        char subkeyName[512] = {};
        DWORD subkeyNameLen = sizeof(subkeyName);
        RegEnumKeyExA(hKey, i, subkeyName, &subkeyNameLen, nullptr, nullptr,
            nullptr, nullptr);

        HKEY hSubkey = nullptr;
        result = RegOpenKeyExA(hKey, subkeyName, 0, KEY_READ, &hSubkey);
        if (result != ERROR_SUCCESS || hSubkey == nullptr) {
            continue;
        }

        DWORD valueCount = 0;
        RegEnumValueA(hSubkey, 0, nullptr, nullptr, nullptr, nullptr,
            nullptr, &valueCount);

        for (DWORD j = 0; j < valueCount; ++j) {
            char valueName[256] = {};
            DWORD valueNameLen = sizeof(valueName);
            DWORD valueType = 0;
            DWORD dataSize = 0;
            char valueData[4096] = {};

            RegEnumValueA(hSubkey, j, valueName, &valueNameLen, nullptr,
                &valueType, valueData, &dataSize);

            RegistryValue regVal;
            regVal.hive = hive;
            regVal.key = key + "\\" + subkeyName;
            regVal.name = valueName;
            regVal.value = std::string(valueData, dataSize);
            regVal.type = getTypeName(valueType);

            values.push_back(regVal);
        }

        RegCloseKey(hSubkey);
    }

    RegCloseKey(hKey);
    return values;
}


std::string getTypeName(DWORD type) {
    switch (type) {
        case REG_SZ:  return "REG_SZ";
        case REG_EXPAND_SZ: return "REG_EXPAND_SZ";
        case REG_BINARY: return "REG_BINARY";
        case REG_DWORD: return "REG_DWORD";
        case REG_DWORD_BIG_ENDIAN: return "REG_DWORD_BIG_ENDIAN";
        case REG_DWORD_LITTLE_ENDIAN: return "REG_DWORD_LITTLE_ENDIAN";
        case REG_LINK: return "REG_LINK";
        case REG_MULTI_SZ: return "REG_MULTI_SZ";
        case REG_RESOURCE_LIST: return "REG_RESOURCE_LIST";
        case REG_FULL_RESOURCE_DESCRIPTOR: return "REG_FULL_RESOURCE_DESCRIPTOR";
        case REG_RESOURCE_REQUIREMENTS_LIST: return "REG_RESOURCE_REQUIREMENTS_LIST";
        case REG_QWORD: return "REG_QWORD";
        default: return "UNKNOWN";
    }
}

std::vector<RegistryValue> RegistryCollector::collectEnvironmentRegistry() {
    std::vector<RegistryValue> values;
    std::string baseKey = "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";

    auto envValues = collectRegistryKey("HKEY_LOCAL_MACHINE", baseKey);
    for (auto& v : envValues) {
        if (v.name.empty() || v.name[0] == ' ') continue;
        values.push_back(v);
    }

    return values;
}

std::vector<RegistryValue> RegistryCollector::collectStartupEntries() {
    std::vector<RegistryValue> values;

    auto run32 = collectRegistryKey("HKEY_LOCAL_MACHINE",
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
    for (auto& v : run32) {
        if (v.name.empty() || v.name[0] == ' ') continue;
        values.push_back(v);
    }

    auto run64 = collectRegistryKey("HKEY_LOCAL_MACHINE",
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
    for (auto& v : run64) {
        if (v.name.empty() || v.name[0] == ' ') continue;
        values.push_back(v);
    }

    return values;
}

}  // namespace weirdsys
