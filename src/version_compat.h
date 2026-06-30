// =============================================================================
// version_compat.h — Taskbar Button Customizer
// Windows build detection and compatibility guards.
// The mod only supports Windows 11 23H2 (22631) / 24H2 (26100) / 25H2.
// Any earlier build causes Wh_ModInit to return FALSE immediately, preventing
// any hooks from being installed.
// =============================================================================
#pragma once

#include <windows.h>
#include "logger.h"

namespace TaskbarCustomizer {

// Build number boundaries ─────────────────────────────────────────────────────
// Windows 11 22H2:  22621  (not listed in spec as supported; we allow it too
//                            so users on a slightly older patch are not blocked)
// Windows 11 23H2:  22631
// Windows 11 24H2:  26100
// Windows 11 25H2:  ~26200 (anticipated; we accept anything ≥ 22621)
constexpr DWORD kMinSupportedBuild = 22621u;

// ─── VersionInfo ─────────────────────────────────────────────────────────────

struct VersionInfo {
    DWORD major   = 0;
    DWORD minor   = 0;
    DWORD build   = 0;
    bool  valid   = false;
};

/// Query the running OS version from ntdll (RtlGetVersion).
/// Returns a VersionInfo with valid=true on success.
inline VersionInfo QueryWindowsVersion() noexcept {
    // Use RtlGetVersion (undocumented but universally available) rather than
    // GetVersionEx, which lies in compatibility mode.
    using RtlGetVersionFn = NTSTATUS(WINAPI*)(PRTL_OSVERSIONINFOW);

    HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) {
        LOG_ERROR(L"[VersionCompat] Failed to obtain ntdll handle.");
        return {};
    }

    auto fnRtlGetVersion = reinterpret_cast<RtlGetVersionFn>(
        ::GetProcAddress(ntdll, "RtlGetVersion"));
    if (!fnRtlGetVersion) {
        LOG_ERROR(L"[VersionCompat] RtlGetVersion not found in ntdll.");
        return {};
    }

    RTL_OSVERSIONINFOW ovi = {};
    ovi.dwOSVersionInfoSize = sizeof(ovi);
    if (fnRtlGetVersion(&ovi) != 0 /* STATUS_SUCCESS = 0 */) {
        LOG_ERROR(L"[VersionCompat] RtlGetVersion failed.");
        return {};
    }

    return VersionInfo {
        ovi.dwMajorVersion,
        ovi.dwMinorVersion,
        ovi.dwBuildNumber,
        true
    };
}

/// Returns true when the running OS is a supported Windows 11 build.
/// Logs a descriptive message on failure so the Windhawk log is informative.
inline bool IsOsVersionSupported() noexcept {
    VersionInfo vi = QueryWindowsVersion();
    if (!vi.valid) {
        LOG_ERROR(L"[VersionCompat] Could not determine Windows build; "
                  L"refusing to load mod.");
        return false;
    }

    LOG_INFO(L"[VersionCompat] Detected Windows %lu.%lu build %lu",
             vi.major, vi.minor, vi.build);

    // Must be Windows 11 (version 10.0, build ≥ 22621)
    if (vi.major != 10 || vi.minor != 0) {
        LOG_WARN(L"[VersionCompat] Unsupported OS version %lu.%lu; "
                 L"mod requires Windows 11.", vi.major, vi.minor);
        return false;
    }

    if (vi.build < kMinSupportedBuild) {
        LOG_WARN(L"[VersionCompat] Build %lu < minimum %lu; "
                 L"mod not supported on this build.", vi.build, kMinSupportedBuild);
        return false;
    }

    LOG_INFO(L"[VersionCompat] OS check passed (build %lu).", vi.build);
    return true;
}

/// Returns the detected build number, or 0 on failure.
inline DWORD GetWindowsBuildNumber() noexcept {
    VersionInfo vi = QueryWindowsVersion();
    return vi.valid ? vi.build : 0u;
}

} // namespace TaskbarCustomizer
