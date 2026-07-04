For a Win32 diagnostic tool comparing system config against a "known good" baseline, here's the landscape of what you can read and how:

## Registry
The biggest source of configuration drift. Use `RegOpenKeyEx`/`RegQueryValueEx` (or `RegGetValue` for simpler one-shot reads). Key areas worth checking:
- `HKLM\SYSTEM\CurrentControlSet\Control` — device/driver settings, boot config
- `HKLM\SYSTEM\CurrentControlSet\Services` — service configs, start types, driver load order
- `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Run*` — startup entries
- `HKLM\SOFTWARE\Policies` and `HKCU\SOFTWARE\Policies` — Group Policy–pushed settings
- `HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment` — system environment variables

## WMI (via COM)
`IWbemLocator`/`IWbemServices` with WQL queries gets you a huge amount of structured info without touching raw registry/API calls:
- `Win32_OperatingSystem` — OS version, install date, locale
- `Win32_ComputerSystem` — domain membership, RAM, manufacturer
- `Win32_Service` — service states in one query
- `Win32_Product` / `Win32_InstalledWin32Program` — installed software (careful, `Win32_Product` is slow and can trigger MSI reconfigs — prefer registry uninstall keys instead)
- `Win32_SystemDriver` — driver load state
- `Win32_NetworkAdapterConfiguration` — network settings
- `Win32_QuickFixEngineering` — installed hotfixes/patches

## Direct Win32 APIs (faster than WMI for these specific bits)
- `GetSystemInfo` / `GetNativeSystemInfo` — CPU architecture, processor count
- `GetVersionEx` is deprecated; use `RtlGetVersion` (via `ntdll.dll`, undocumented but stable) or check `kernel32.dll` version resource for actual OS build
- `GetSystemMetrics` — UI/display config (SM_CXSCREEN, SM_CMONITORS, etc.)
- `GetSystemPowerStatus` — power/battery config
- `GetComputerNameEx` / `GetUserName` — identity
- `GetLocaleInfoEx` / `GetSystemDefaultLangID` — locale/language settings
- `EnumSystemFirmwareTables` / `GetSystemFirmwareTable` — SMBIOS/ACPI table dumps (great for hardware-level diffs)
- `NetWkstaGetInfo` / `NetServerGetInfo` (Netapi32) — domain/workgroup role info

## Services & Drivers
`OpenSCManager` + `EnumServicesStatusEx` gives you every service, its start type, and current state in one pass — often more reliable than parsing WMI for this.

## Environment & Path
`GetEnvironmentStrings` / `GetEnvironmentVariable` for both process and (via registry) system-wide defaults, since these commonly drift between machines.

## Group Policy
`RSoP` via WMI (`root\rsop\computer` namespace) if you want to see *effective* policy rather than just registry values — useful because policy can override registry values you'd otherwise flag as "wrong."

## File system / drivers on disk
`EnumDeviceDrivers` + `GetDeviceDriverFileName` (psapi.h) to enumerate loaded kernel drivers and cross-check versions via `GetFileVersionInfo`.

---

**Practical approach for a diff tool:** I'd build a "collector" module using mostly registry reads + a handful of WMI queries (services, hotfixes, installed software) and direct API calls only where WMI is noticeably slower (WMI has real per-query overhead, ~10-50ms+ per query due to COM marshaling). Serialize your baseline snapshot to JSON or a simple key-value format, then diff against it on demand.

Want me to sketch out the actual C/C++ code structure for the registry + WMI collectors, or would a specific area (services, drivers, network) be more useful to start with?
