#pragma once

#include "types.h"
#include <string>
#include <vector>

namespace weirdsys {

class Collector {
public:
    virtual ~Collector() = default;
    virtual std::vector<CollectedData> collect() = 0;

protected:
    static constexpr const char* REGISTRY_PATHS[] = {
        "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control",
        "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services",
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies",
        "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
        nullptr
    };

    static constexpr const char* WMI_CLASSES[] = {
        "Win32_OperatingSystem",
        "Win32_ComputerSystem",
        "Win32_Service",
        "Win32_QuickFixEngineering",
        "Win32_SystemDriver",
        "Win32_NetworkAdapterConfiguration",
        nullptr
    };
};

class RegistryCollector : public Collector {
public:
    RegistryCollector();
    virtual ~RegistryCollector() = default;
    std::vector<CollectedData> collect() override;

private:
    std::vector<RegistryValue> collectRegistryKey(const std::string& hive,
                                                   const std::string& key);
    std::vector<RegistryValue> collectEnvironmentRegistry();
    std::vector<RegistryValue> collectStartupEntries();
};

class WmiCollector : public Collector {
public:
    WmiCollector();
    virtual ~WmiCollector() = default;
    std::vector<CollectedData> collect() override;

private:
    WmiResult executeWmiQuery(const std::string& wql);
    WmiResult queryOperatingSystem();
    WmiResult queryComputerSystem();
    WmiResult queryServices();
    WmiResult queryHotfixes();
    WmiResult queryNetworkAdapters();
    WmiResult queryDrivers();

    // Helper for WMI data extraction
    static std::vector<CollectedData> extractWmiData(const WmiResult& result);
};

class SystemCollector : public Collector {
public:
    SystemCollector();
    virtual ~SystemCollector() = default;
    std::vector<CollectedData> collect() override;

private:
    std::vector<CollectedData> collectCpuInfo();
    std::vector<CollectedData> collectDisplayInfo();
    std::vector<CollectedData> collectPowerInfo();
    std::vector<CollectedData> collectIdentityInfo();
    std::vector<CollectedData> collectLocaleInfo();
    std::vector<CollectedData> collectFirmwareInfo();
    std::vector<CollectedData> collectKernelDrivers();
    std::vector<CollectedData> collectEnvironmentVars();
    std::vector<CollectedData> collectWindowsVersion();
    std::vector<CollectedData> collectDomainInfo();
};

}  // namespace weirdsys
