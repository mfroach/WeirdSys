# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a Win32 diagnostic tool that compares a Windows system's current configuration against a "known good" baseline to detect configuration drift. The tool is built in C/C++ and uses Win32 APIs, WMI (via COM), and direct registry access to collect system information.

## Architecture

The project is structured around three main functional modules:

### Collector Module (`src/collector/`)

Responsible for gathering system configuration data from various Windows APIs:

**Registry Collection:**
- `HKLM\SYSTEM\CurrentControlSet\Control` - device/driver settings, boot config
- `HKLM\SYSTEM\CurrentControlSet\Services` - service configs, start types, driver load order
- `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Run*` - startup entries
- `HKLM\SOFTWARE\Policies` and `HKCU\SOFTWARE\Policies` - Group Policy–pushed settings
- `HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment` - system environment variables

**WMI Collection (via COM):**
- `Win32_OperatingSystem` - OS version, install date, locale
- `Win32_ComputerSystem` - domain membership, RAM, manufacturer
- `Win32_Service` - service states in one query
- `Win32_Product` / `Win32_InstalledWin32Program` - installed software
- `Win32_SystemDriver` - driver load state
- `Win32_NetworkAdapterConfiguration` - network settings
- `Win32_QuickFixEngineering` - installed hotfixes/patches

**Direct Win32 APIs (faster than WMI for specific data):**
- `GetSystemInfo` / `GetNativeSystemInfo` - CPU architecture, processor count
- `RtlGetVersion` (via ntdll.dll) or `kernel32.dll` version resource for OS build
- `GetSystemMetrics` - UI/display config (SM_CXSCREEN, SM_CMONITORS, etc.)
- `GetSystemPowerStatus` - power/battery config
- `GetComputerNameEx` / `GetUserName` - identity
- `GetLocaleInfoEx` / `GetSystemDefaultLangID` - locale/language settings
- `EnumSystemFirmwareTables` / `GetSystemFirmwareTable` - SMBIOS/ACPI table dumps
- `NetWkstaGetInfo` / `NetServerGetInfo` (Netapi32) - domain/workgroup role info
- `OpenSCManager` + `EnumServicesStatusEx` - service enumeration (more reliable than WMI for services)
- `GetEnvironmentStrings` / `GetEnvironmentVariable` - environment variables
- `EnumDeviceDrivers` + `GetDeviceDriverFileName` (psapi.h) - loaded kernel drivers with version checks via `GetFileVersionInfo`

### Diff Module (`src/diff/`)

Compares collected system state against a baseline snapshot, identifying configuration drift.

### Report Module (`src/report/`)

Generates human-readable or machine-parseable reports showing:
- Baseline snapshot (serialized to JSON or key-value format)
- Detected differences between current and baseline states
- Severity or impact categorization of drifts

## Development Approach

1. **Collector Design**: Build a "collector" module that uses mostly registry reads + a handful of WMI queries (WMI has real per-query overhead, ~10-50ms+ per query due to COM marshaling). Use direct API calls only where WMI is noticeably slower.

2. **Data Format**: Serialize baseline snapshots to JSON or a simple key-value format for easy comparison and storage.

3. **Diff Strategy**: Diff against the serialized baseline on demand rather than comparing raw registry values.

## Current State

The codebase is currently in the planning/early development phase. The `src/collector/`, `src/diff/`, `src/report/`, and `include/` directories exist but are empty. Source code implementation needs to be created based on the architecture outlined above.

## Build System

The project uses traditional Win32 development patterns. The build system needs to be determined based on the preferred approach (Visual Studio, CMake, or another build system).
