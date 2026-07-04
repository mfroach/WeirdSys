#include "weirdsys/collector.h"
#include <algorithm>
#include <string>
#include <vector>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <wbemcli.h>
#include <wql.h>
#include <initguid.h>
#include <wbemidl.h>
#include <comdef.h>

// Helper function to convert BSTR to std::string
static std::string bstrToString(BSTR bstr) {
    if (bstr == nullptr) return "";
    size_t len = SysStringLen(bstr);
    return std::string(bstr, len);
}

// Helper function to convert VARIANT to std::string
static std::string variantToString(VARIANT* pVar) {
    if (pVar == nullptr) return "";

    switch (pVar->vt) {
        case VT_BSTR:
            return bstrToString(pVar->bstrVal);
        case VT_LPSTR:
            return std::string(pVar->lpstrVal);
        case VT_LPWSTR:
            if (pVar->pwszVal) {
                size_t len = SysStringLen(pVar->pwszVal);
                return std::string(pVar->pwszVal, len);
            }
            return "";
        case VT_EMPTY:
            return "";
        case VT_UI4:  // Uint
            return std::to_string(pVar->ulVal);
        case VT_I4:   // Int
            return std::to_string(pVar->lVal);
        case VT_R8:   // Double
            return std::to_string(pVar->dblVal);
        default:
            return "[unknown type]";
    }
}

// Helper function to convert WmiResult into CollectedData
static std::vector<weirdsys::CollectedData> extractWmiData(const WmiResult& result) {
    std::vector<weirdsys::CollectedData> data;

    // Return early if no properties or values
    if (result.properties.empty() || result.values.empty()) {
        return data;
    }

    // Number of columns = min(properties.size(), values.size())
    size_t numColumns = std::min(result.properties.size(), result.values.size());
    if (numColumns == 0) {
        return data;
    }

    // Each row represents an instance with columns as properties
    for (size_t col = 0; col < numColumns; ++col) {
        for (size_t row = 0; row < result.values.size(); ++row) {
            std::string key = result.properties[col];
            std::string value = variantToString(&result.values[row]);

            if (!key.empty() && !value.empty()) {
                weirdsys::CollectedData item;
                item.key = key;
                item.value = value;
                data.push_back(item);
            }
        }
    }

    return data;
}

WmiResult WmiCollector::executeWmiQuery(const std::string& wql) {
    WmiResult result;

    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM: " << hr << std::endl;
        return result;
    }

    try {
        // Create WbemLocator using safe array
        IWbemLocator* pLocator = nullptr;
        hr = CoCreateInstance(CLSID_WbemLocator, nullptr,
                               CLSCTX_INPROC_SERVER, IID_IWbemLocator,
                               reinterpret_cast<void**>(&pLocator));
        if (FAILED(hr)) {
            std::cerr << "Failed to create WbemLocator: " << hr << std::endl;
            CoUninitialize();
            return result;
        }

        // Initialize WbemLocator
        hr = pLocator->Initialize(L"root\\cimv2", 0, 0, nullptr, nullptr);
        if (FAILED(hr)) {
            std::cerr << "Failed to initialize WbemLocator: " << hr << std::endl;
            pLocator->Release();
            CoUninitialize();
            return result;
        }

        // Set security levels
        hr = CoSetProxyBlanket(pLocator, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               nullptr, RPC_C_AUTHN_LEVEL_CALL,
                               RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
        if (FAILED(hr)) {
            std::cerr << "Failed to set proxy blanket: " << hr << std::endl;
            pLocator->Release();
            CoUninitialize();
            return result;
        }

        // Connect to WMI
        IWbemServices* pServices = nullptr;
        hr = pLocator->ConnectServer(L"root\\CIMV2", nullptr, nullptr, 0,
                                     nullptr, nullptr, nullptr, nullptr,
                                     &pServices);
        if (FAILED(hr)) {
            std::cerr << "Failed to connect to WMI: " << hr << std::endl;
            pLocator->Release();
            CoUninitialize();
            return result;
        }

        // Set default security on services
        hr = CoSetProxyBlanket(pServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               nullptr, RPC_C_AUTHN_LEVEL_CALL,
                               RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
        if (FAILED(hr)) {
            std::cerr << "Failed to set proxy blanket on services: " << hr << std::endl;
            pServices->Release();
            pLocator->Release();
            CoUninitialize();
            return result;
        }

        // Execute the WQL query
        IWbemClassObject* pClass = nullptr;
        hr = pServices->ExecQuery(L"WQL", wql.c_str(), 0, nullptr,
                                  nullptr, nullptr, &pClass);
        if (FAILED(hr)) {
            std::cerr << "Failed to execute query: " << hr << std::endl;
            pServices->Release();
            pLocator->Release();
            CoUninitialize();
            return result;
        }

        // Get the class name
        BSTR bClassName = nullptr;
        hr = pClass->Get(L"__CLASS", nullptr, &bClassName, nullptr, nullptr);
        if (SUCCEEDED(hr)) {
            result.class_name = bstrToString(bClassName);
            SysFreeString(bClassName);
        }

        // Get property names
        BSTR bPropertyNames = nullptr;
        hr = pClass->Get(L"__PROPERTY_NAMES", nullptr, &bPropertyNames, nullptr, nullptr);
        if (SUCCEEDED(hr) && bPropertyNames) {
            size_t propLen = SysStringLen(bPropertyNames);
            result.properties.reserve(propLen + 10);
            for (size_t i = 0; i < propLen; ++i) {
                if (bPropertyNames[i] == L',') {
                    result.properties.push_back(' ');
                } else {
                    result.properties.push_back(bPropertyNames[i]);
                }
            }
            SysFreeString(bPropertyNames);
        }

        // Get instance count
        long instanceCount = 0;
        hr = pClass->Get(L"__INSTANCES_COUNT", nullptr, &instanceCount, nullptr, nullptr);
        if (SUCCEEDED(hr) && instanceCount > 0) {
            // Execute an enumeration query to get actual instances
            BSTR wqlWithLimit = L"SELECT * FROM " + std::wstring(wql.begin(), wql.end()) + L";";
            IWbemClassObject* pInstance = nullptr;
            hr = pServices->ExecQuery(wqlWithLimit.c_str(), 0, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                      nullptr, &pInstance);
            if (SUCCEEDED(hr) && pInstance) {
                IWbemArrayBase* pArray = nullptr;
                hr = pInstance->GetObjectArrayParam(1, &pArray);
                if (SUCCEEDED(hr) && pArray) {
                    long count = 0;
                    hr = pArray->GetCount(&count);
                    if (SUCCEEDED(hr) && count > 0) {
                        // Get property values for each instance
                        for (long i = 0; i < (long)result.properties.size() && i < (long)result.values.size(); ++i) {
                            BSTR bPropertyName = SysAllocString(result.properties[i].c_str());
                            BSTR bPropertyValue = nullptr;

                            hr = pInstance->GetProperty(bPropertyName, nullptr, &bPropertyValue, nullptr, nullptr);
                            if (SUCCEEDED(hr) && bPropertyValue) {
                                std::string propValue = bstrToString(bPropertyValue);
                                result.values.push_back(propValue);
                                SysFreeString(bPropertyValue);
                            }
                            SysFreeString(bPropertyName);
                        }
                    }
                    pArray->Release();
                }
                pInstance->Release();
            }
        }

        pClass->Release();
        pServices->Release();
        pLocator->Release();

    } catch (const _com_error& e) {
        std::cerr << "COM Error: " << e.ErrorMessage() << std::endl;
    }

    CoUninitialize();
    return result;
}

std::vector<CollectedData> WmiCollector::collect() {
    std::vector<CollectedData> data;

    auto os_data = queryOperatingSystem();
    data.insert(data.end(), extractWmiData(os_data).begin(), extractWmiData(os_data).end());

    auto cs_data = queryComputerSystem();
    data.insert(data.end(), extractWmiData(cs_data).begin(), extractWmiData(cs_data).end());

    auto services_data = queryServices();
    data.insert(data.end(), extractWmiData(services_data).begin(), extractWmiData(services_data).end());

    auto hotfix_data = queryHotfixes();
    data.insert(data.end(), extractWmiData(hotfix_data).begin(), extractWmiData(hotfix_data).end());

    auto network_data = queryNetworkAdapters();
    data.insert(data.end(), extractWmiData(network_data).begin(), extractWmiData(network_data).end());

    auto driver_data = queryDrivers();
    data.insert(data.end(), extractWmiData(driver_data).begin(), extractWmiData(driver_data).end());

    return data;
}

WmiResult WmiCollector::queryOperatingSystem() {
    return executeWmiQuery(L"SELECT Caption, Version, BuildNumber, CSName, InstallDate FROM Win32_OperatingSystem");
}

WmiResult WmiCollector::queryComputerSystem() {
    return executeWmiQuery(L"SELECT Name, Domain, Model, Manufacturer, TotalMemory, NumberOfProcesses FROM Win32_ComputerSystem");
}

WmiResult WmiCollector::queryServices() {
    return executeWmiQuery(L"SELECT Name, DisplayName, State, StartMode, ProcessId, PathName FROM Win32_Service");
}

WmiResult WmiCollector::queryHotfixes() {
    return executeWmiQuery(L"SELECT CSQ, KBNumber, InstalledOn, Description, InstalledBy FROM Win32_QuickFixEngineering");
}

WmiResult WmiCollector::queryNetworkAdapters() {
    return executeWmiQuery(L"SELECT Name, MACAddress, IPAddress, SubnetMask, DefaultIPGateway FROM Win32_NetworkAdapterConfiguration WHERE IPEnabled = TRUE");
}

WmiResult WmiCollector::queryDrivers() {
    return executeWmiQuery(L"SELECT Name, DriverVersion, DriverDate, ProviderName, LoadOrder FROM Win32_SystemDriver");
}

std::vector<CollectedData> WmiCollector::extractWmiData(const WmiResult& result) {
    std::vector<CollectedData> data;

    if (result.properties.empty() || result.values.empty()) {
        return data;
    }

    size_t numColumns = std::min(result.properties.size(), result.values.size());
    if (numColumns == 0) {
        return data;
    }

    for (size_t col = 0; col < numColumns; ++col) {
        for (size_t row = 0; row < result.values.size(); ++row) {
            std::string key = result.properties[col];
            std::string value = variantToString(&result.values[row]);

            if (!key.empty() && !value.empty()) {
                CollectedData item;
                item.key = key;
                item.value = value;
                data.push_back(item);
            }
        }
    }

    return data;
}

WmiResult WmiCollector::executeWmiQuery(const std::string& wql) {
    WmiResult result;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        return result;
    }

    try {
        IWbemLocator* pLocator = nullptr;
        hr = CoCreateInstance(CLSID_WbemLocator, nullptr,
                               CLSCTX_INPROC_SERVER, IID_IWbemLocator,
                               reinterpret_cast<void**>(&pLocator));
        if (FAILED(hr)) {
            CoUninitialize();
            return result;
        }

        hr = pLocator->Initialize(L"root\\cimv2", 0, 0, nullptr, nullptr);
        if (FAILED(hr)) {
            pLocator->Release();
            CoUninitialize();
            return result;
        }

        hr = CoSetProxyBlanket(pLocator, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               nullptr, RPC_C_AUTHN_LEVEL_CALL,
                               RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
        if (FAILED(hr)) {
            pLocator->Release();
            CoUninitialize();
            return result;
        }

        IWbemServices* pServices = nullptr;
        hr = pLocator->ConnectServer(L"root\\CIMV2", nullptr, nullptr, 0,
                                     nullptr, nullptr, nullptr, nullptr,
                                     &pServices);
        if (FAILED(hr)) {
            pLocator->Release();
            CoUninitialize();
            return result;
        }

        hr = CoSetProxyBlanket(pServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                               nullptr, RPC_C_AUTHN_LEVEL_CALL,
                               RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
        if (FAILED(hr)) {
            pServices->Release();
            pLocator->Release();
            CoUninitialize();
            return result;
        }

        IWbemClassObject* pClass = nullptr;
        hr = pServices->ExecQuery(L"WQL", wql.c_str(), 0, nullptr,
                                  nullptr, nullptr, &pClass);
        if (FAILED(hr)) {
            pServices->Release();
            pLocator->Release();
            CoUninitialize();
            return result;
        }

        BSTR bClassName = nullptr;
        hr = pClass->Get(L"__CLASS", nullptr, &bClassName, nullptr, nullptr);
        if (SUCCEEDED(hr)) {
            result.class_name = bstrToString(bClassName);
            SysFreeString(bClassName);
        }

        BSTR bPropertyNames = nullptr;
        hr = pClass->Get(L"__PROPERTY_NAMES", nullptr, &bPropertyNames, nullptr, nullptr);
        if (SUCCEEDED(hr) && bPropertyNames) {
            size_t propLen = SysStringLen(bPropertyNames);
            result.properties.reserve(propLen + 10);
            for (size_t i = 0; i < propLen; ++i) {
                if (bPropertyNames[i] == L',') {
                    result.properties.push_back(' ');
                } else {
                    result.properties.push_back(bPropertyNames[i]);
                }
            }
            SysFreeString(bPropertyNames);
        }

        long instanceCount = 0;
        hr = pClass->Get(L"__INSTANCES_COUNT", nullptr, &instanceCount, nullptr, nullptr);
        if (SUCCEEDED(hr) && instanceCount > 0) {
            BSTR wqlWithLimit = L"SELECT * FROM " + std::wstring(wql.begin(), wql.end()) + L";";
            IWbemClassObject* pInstance = nullptr;
            hr = pServices->ExecQuery(wqlWithLimit.c_str(), 0, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                      nullptr, &pInstance);
            if (SUCCEEDED(hr) && pInstance) {
                IWbemArrayBase* pArray = nullptr;
                hr = pInstance->GetObjectArrayParam(1, &pArray);
                if (SUCCEEDED(hr) && pArray) {
                    long count = 0;
                    hr = pArray->GetCount(&count);
                    if (SUCCEEDED(hr) && count > 0) {
                        for (long i = 0; i < (long)result.properties.size() && i < (long)result.values.size(); ++i) {
                            BSTR bPropertyName = SysAllocString(result.properties[i].c_str());
                            BSTR bPropertyValue = nullptr;

                            hr = pInstance->GetProperty(bPropertyName, nullptr, &bPropertyValue, nullptr, nullptr);
                            if (SUCCEEDED(hr) && bPropertyValue) {
                                std::string propValue = bstrToString(bPropertyValue);
                                result.values.push_back(propValue);
                                SysFreeString(bPropertyValue);
                            }
                            SysFreeString(bPropertyName);
                        }
                    }
                    pArray->Release();
                }
                pInstance->Release();
            }
        }

        pClass->Release();
        pServices->Release();
        pLocator->Release();

    } catch (const _com_error& e) {
    }

    CoUninitialize();
    return result;
}

std::string WmiCollector::bstrToString(BSTR bstr) {
    if (bstr == nullptr) return "";
    size_t len = SysStringLen(bstr);
    return std::string(bstr, len);
}

std::string WmiCollector::variantToString(VARIANT* pVar) {
    if (pVar == nullptr) return "";

    switch (pVar->vt) {
        case VT_BSTR:
            return bstrToString(pVar->bstrVal);
        case VT_LPSTR:
            return std::string(pVar->lpstrVal);
        case VT_LPWSTR:
            if (pVar->pwszVal) {
                size_t len = SysStringLen(pVar->pwszVal);
                return std::string(pVar->pwszVal, len);
            }
            return "";
        case VT_EMPTY:
            return "";
        case VT_UI4:
            return std::to_string(pVar->ulVal);
        case VT_I4:
            return std::to_string(pVar->lVal);
        case VT_R8:
            return std::to_string(pVar->dblVal);
        default:
            return "[unknown type]";
    }
}

}  // namespace weirdsys
