#include "weirdsys/collector.h"
#include <algorithm>
#include <windows.h>

namespace weirdsys {

SystemCollector::SystemCollector() {}

SystemCollector::~SystemCollector() = default;

std::vector<CollectedData> SystemCollector::collect() {
    std::vector<CollectedData> data;

    data.insert(data.end(), collectCpuInfo().begin(), collectCpuInfo().end());
    data.insert(data.end(), collectDisplayInfo().begin(), collectDisplayInfo().end());
    data.insert(data.end(), collectPowerInfo().begin(), collectPowerInfo().end());
    data.insert(data.end(), collectIdentityInfo().begin(), collectIdentityInfo().end());
    data.insert(data.end(), collectLocaleInfo().begin(), collectLocaleInfo().end());
    data.insert(data.end(), collectFirmwareInfo().begin(), collectFirmwareInfo().end());
    data.insert(data.end(), collectKernelDrivers().begin(), collectKernelDrivers().end());
    data.insert(data.end(), collectEnvironmentVars().begin(), collectEnvironmentVars().end());
    data.insert(data.end(), collectWindowsVersion().begin(), collectWindowsVersion().end());
    data.insert(data.end(), collectDomainInfo().begin(), collectDomainInfo().end());

    return data;
}

std::vector<CollectedData> SystemCollector::collectCpuInfo() {
    std::vector<CollectedData> data;

    SYSTEM_INFO si;
    GetSystemInfo(&si);

    data.push_back(CollectedData{"CPU_Type", "Unknown"});
    if (si.wProcessorArchitecture == PROCESSOR_INTEL_64)
        data.back().value = "Intel 64";
    else if (si.wProcessorArchitecture == PROCESSOR_AMD_X8664)
        data.back().value = "AMD x64";
    else if (si.wProcessorArchitecture == PROCESSOR_ARM64)
        data.back().value = "ARM64";
    else
        data.back().value = std::to_string(si.wProcessorArchitecture);

    data.push_back(CollectedData{"CPU_Count", std::to_string(si.dwNumberOfProcessors)});
    data.push_back(CollectedData{"CPU_Interrupter_Count", std::to_string(si.dwNumberOfProcessors)});

    NATIVE_SYSTEM_INFO nsi;
    GetNativeSystemInfo(&nsi);
    data.push_back(CollectedData{"CPU_Native_Count", std::to_string(nsi.dwNumberOfProcessors)});

    return data;
}

std::vector<CollectedData> SystemCollector::collectDisplayInfo() {
    std::vector<CollectedData> data;

    data.push_back(CollectedData{"SM_CXSCREEN", std::to_string(GetSystemMetrics(SM_CXSCREEN))});
    data.push_back(CollectedData{"SM_CYSCREEN", std::to_string(GetSystemMetrics(SM_CYSCREEN))});
    data.push_back(CollectedData{"SM_CMONITORS", std::to_string(GetSystemMetrics(SM_CMONITORS))});
    data.push_back(CollectedData{"SM_CXVIRTUALSCREEN", std::to_string(GetSystemMetrics(SM_CXVIRTUALSCREEN))});
    data.push_back(CollectedData{"SM_CYVIRTUALSCREEN", std::to_string(GetSystemMetrics(SM_CYVIRTUALSCREEN))});
    data.push_back(CollectedData{"SM_CXWORKINGAREA", std::to_string(GetSystemMetrics(SM_CXWORKINGAREA))});
    data.push_back(CollectedData{"SM_CYWORKINGAREA", std::to_string(GetSystemMetrics(SM_CYWORKINGAREA))});

    return data;
}

std::vector<CollectedData> SystemCollector::collectPowerInfo() {
    std::vector<CollectedData> data;

    SYSTEM_POWER_STATUS sps;
    GetSystemPowerStatus(&sps);

    data.push_back(CollectedData{"Power_Status", sps.ACLineStatus == AC_ONLINE ? "AC Power" : "Battery Power"});
    data.push_back(CollectedData{"Power_BatteryFlag", sps.BatteryFlag == BATTERY_FLAG_HIGH ? "Good" :
        sps.BatteryFlag == BATTERY_FLAG_LOW ? "Low" :
        sps.BatteryFlag == BATTERY_FLAG_CRITICAL ? "Critical" : "Unknown"});
    data.push_back(CollectedData{"Power_BatteryLevel", std::to_string(sps.BatteryLevel)});
    data.push_back(CollectedData{"Power_MajorState", sps.MajorState == POWER_ALTERNATE ? "Alternate" : "Main"});
    data.push_back(CollectedData{"Power_MinuteRemaining", std::to_string(sps.MinuteRemaining)});
    data.push_back(CollectedData{"Power_PercentageFull", std::to_string(sps.PercentFull)});
    data.push_back(CollectedData{"Power_SystemStateFlag", sps.SystemStateFlag == SYSTEM_STATE_POWER_AC ? "AC" :
        sps.SystemStateFlag == SYSTEM_STATE_POWER_BATTERY ? "Battery" : "Unknown"});

    return data;
}

std::vector<CollectedData> SystemCollector::collectIdentityInfo() {
    std::vector<CollectedData> data;

    char name[256] = {};
    DWORD size = sizeof(name);
    GetComputerNameEx(ComputerNameDnsFullyQualified, name, &size);
    data.push_back(CollectedData{"ComputerName_FullDNS", name});

    char name2[256] = {};
    GetComputerNameEx(ComputerNameNetBIOS, name2, &size);
    data.push_back(CollectedData{"ComputerName_NetBIOS", name2});

    char user[256] = {};
    DWORD size2 = sizeof(user);
    GetUserNameA(user, &size2);
    data.push_back(CollectedData{"UserName", user});

    char domain[256] = {};
    DWORD size3 = sizeof(domain);
    GetComputerNameEx(ComputerNameDomainDNS, domain, &size3);
    data.push_back(CollectedData{"Domain_DNS", domain});

    data.push_back(CollectedData{"Domain_NetBIOS", ""});

    // Hostname is the NetBIOS name (short computer name)
    data.push_back(CollectedData{"Hostname", name2});

    return data;
}

std::vector<CollectedData> SystemCollector::collectLocaleInfo() {
    std::vector<CollectedData> data;

    data.push_back(CollectedData{"LCID", std::to_string(GetSystemDefaultLCID())});
    data.push_back(CollectedData{"Locale_String", GetLocaleInfoA(LOCALE_SYSTEM_DEFAULT, LOCALE_SNAME, nullptr, 0)});
    data.push_back(CollectedData{"Locale_LangID", std::to_string(GetSystemDefaultLangID())});
    data.push_back(CollectedData{"Locale_CountryID", std::to_string(GetSystemDefaultLCID())});

    wchar_t wcName[256] = {};
    DWORD wcsSize = sizeof(wcName);
    GetComputerNameExW(ComputerNameDnsFullyQualified, wcName, &wcsSize);
    data.push_back(CollectedData{"ComputerName_Wide", wcsName});

    return data;
}

std::vector<CollectedData> SystemCollector::collectFirmwareInfo() {
    std::vector<CollectedData> data;

    // EnumSystemFirmwareTables - SMBIOS/ACPI tables
    unsigned char dataBuffer[65536] = {};
    unsigned char dataBuffer2[65536] = {};
    unsigned char dataBuffer3[65536] = {};

    // SMBIOS 1.0
    auto result1 = EnumSystemFirmwareTables(0x0000, L"SMBIOS 1.0", dataBuffer, 65536);
    if (result1 != ERROR_SUCCESS) {
        data.push_back(CollectedData{"SMBIOS_1.0", "Not available"});
    } else {
        data.push_back(CollectedData{"SMBIOS_1.0", "Available"});
    }

    // SMBIOS 2.0
    auto result2 = EnumSystemFirmwareTables(0x0001, L"SMBIOS 2.0", dataBuffer2, 65536);
    if (result2 != ERROR_SUCCESS) {
        data.push_back(CollectedData{"SMBIOS_2.0", "Not available"});
    } else {
        data.push_back(CollectedData{"SMBIOS_2.0", "Available"});
    }

    // SMBIOS 3.0
    auto result3 = EnumSystemFirmwareTables(0x0002, L"SMBIOS 3.0", dataBuffer3, 65536);
    if (result3 != ERROR_SUCCESS) {
        data.push_back(CollectedData{"SMBIOS_3.0", "Not available"});
    } else {
        data.push_back(CollectedData{"SMBIOS_3.0", "Available"});
    }

    // GetSystemFirmwareTable - SMBIOS 2.0
    auto fwResult = GetSystemFirmwareTable(0x0001, L"SMBIOS 2.0", dataBuffer, 65536);
    data.push_back(CollectedData{"FirmwareTable_SMBIOS2", fwResult == ERROR_SUCCESS ? "Retrieved" : "Failed"});

    return data;
}

std::vector<CollectedData> SystemCollector::collectKernelDrivers() {
    std::vector<CollectedData> data;

    HMODULE hMods[1024];
    DWORD cbNeeded;
    EnumProcesses(hMods, sizeof(hMods), &cbNeeded);

    DWORD numModules = cbNeeded / sizeof(HMODULE);

    for (DWORD i = 0; i < numModules; ++i) {
        HMODULE hMod = hMods[i];
        if (hMod == nullptr) continue;

        char path[512] = {};
        DWORD pathLen = GetModuleFileNameExA(hMod, nullptr, path, sizeof(path));
        if (pathLen > 0) {
            data.push_back(CollectedData{"Driver_Module", path});
        }

        FreeLibrary(hMod);
    }

    // GetFileVersionInfo for a sample driver
    char driverPath[512] = {};
    GetModuleFileNameA(nullptr, driverPath, sizeof(driverPath));

    if (driverPath[0] != '\0') {
        HMODULE hVer = FindResourceA(nullptr, MAKEINTRESOURCE(1), RT_VERSION);
        if (hVer) {
            data.push_back(CollectedData{"Driver_Version", "Module loaded"});
        }
    }

    return data;
}

std::vector<CollectedData> SystemCollector::collectEnvironmentVars() {
    std::vector<CollectedData> data;

    LPVOID envStrs = nullptr;
    DWORD envSize = 0;

    // GetEnvironmentStrings returns pointer to all environment strings
    envStrs = GetEnvironmentStringsA();

    if (envStrs != nullptr) {
        char* p = static_cast<char*>(envStrs);
        while (*p != '\0') {
            if (*p == '=' && p[1] != '\0') {
                char name[256] = {}, value[2048] = {};
                int nameLen = 0, valueLen = 0;

                while (*p == ' ' || *p == '\t') p++;
                while (*p && *p != '=') {
                    name[nameLen++] = *p++;
                }
                while (*p == ' ' || *p == '\t') p++;

                if (*p == '\0') break;
                while (*p && *p != '\0') p++;
                p++;

                while (*p == ' ' || *p == '\t') p++;
                while (*p && *p != '\0') {
                    value[valueLen++] = *p++;
                }

                if (nameLen > 0) {
                    data.push_back(CollectedData{name, value});
                }
            }
            p++;
        }
        FreeEnvironmentStringsA(envStrs);
    }

    // Also use GetEnvironmentVariable for specific ones
    const char* vars[] = {"PATH", "TEMP", "TMP", "COMSPEC", "PROMPT", "SYSTEMROOT", nullptr};
    for (int i = 0; vars[i] != nullptr; ++i) {
        char value[4096] = {};
        DWORD size = sizeof(value);
        if (GetEnvironmentVariableA(vars[i], value, size) > 0) {
            data.push_back(CollectedData{vars[i], value});
        }
    }

    return data;
}

std::vector<CollectedData> SystemCollector::collectWindowsVersion() {
    std::vector<CollectedData> data;

    // Registry-based approach - more reliable than WMI
    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        0, KEY_READ, &hKey);

    if (result == ERROR_SUCCESS && hKey != nullptr) {
        char buffer[4096] = {};
        DWORD size = sizeof(buffer);

        // Get product type
        if (RegQueryValueExA(hKey, "ProductType", nullptr, nullptr,
                             buffer, &size) == ERROR_SUCCESS) {
            std::string productType(buffer, size);
            data.push_back(CollectedData{"Windows_ProductType", productType});
        }

        // Get installed display version
        if (RegQueryValueExA(hKey, "ProductName", nullptr, nullptr,
                             buffer, &size) == ERROR_SUCCESS) {
            std::string productName(buffer, size);
            data.push_back(CollectedData{"Windows_ProductName", productName});
        }

        // Get build number
        if (RegQueryValueExA(hKey, "CurrentBuild", nullptr, nullptr,
                             buffer, &size) == ERROR_SUCCESS) {
            std::string build(buffer, size);
            data.push_back(CollectedData{"Windows_BuildNumber", build});
        }

        // Get display version
        if (RegQueryValueExA(hKey, "DisplayVersion", nullptr, nullptr,
                             buffer, &size) == ERROR_SUCCESS) {
            std::string displayVersion(buffer, size);
            data.push_back(CollectedData{"Windows_DisplayVersion", displayVersion});
        }

        RegCloseKey(hKey);
    }

    // Additional registry keys for more detailed info
    hKey = nullptr;
    result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\BuildLab",
        0, KEY_READ, &hKey);

    if (result == ERROR_SUCCESS && hKey != nullptr) {
        if (RegQueryValueExA(hKey, "BuildLabEx", nullptr, nullptr,
                             buffer, &size) == ERROR_SUCCESS) {
            std::string buildLab(buffer, size);
            data.push_back(CollectedData{"Windows_BuildLab", buildLab});
        }
        RegCloseKey(hKey);
    }

    return data;
}

std::vector<CollectedData> SystemCollector::collectDomainInfo() {
    std::vector<CollectedData> data;

    // Method 1: GetComputerNameEx for Primary DNS name (works on joined machines)
    char dnsName[256] = {};
    DWORD dnsSize = sizeof(dnsName);
    if (GetComputerNameExA(ComputerNamePrimaryDnsName, dnsName, &dnsSize) > 0) {
        std::string primaryDns(dnsName, dnsSize);
        data.push_back(CollectedData{"Domain_PrimaryDNSName", primaryDns});
    }

    // Method 2: Netapi32 - GetWorkStationInfo (requires network connection)
    WORKSTATION_INFO workInfo = {};
    if (NetWkstaGetInfo(nullptr, 101, &workInfo) == NERR_Success) {
        // NetWkstaGetInfo with 101 doesn't return full domain info,
        // so we use the primary DNS as fallback
        std::string workgroupName(workInfo.lpNetName, workInfo.nNetNameLen);
        data.push_back(CollectedData{"Domain_WorkgroupName", workgroupName});
    }

    // Method 3: Netapi32 - GetNetServerString (returns domain controller info)
    NETSERVER_STRING serverInfo = {};
    if (NetServerGetInfo(nullptr, 101, &serverInfo) == NERR_Success) {
        std::string domainName(serverInfo.lpDomainName, serverInfo.nDomainNameLen);
        data.push_back(CollectedData{"Domain_Name", domainName});
    }

    return data;
}

}  // namespace weirdsys
