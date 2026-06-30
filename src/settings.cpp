// =============================================================================
// settings.cpp — Taskbar Button Customizer
// Implementation of SettingsManager: reads, validates, and provides profiles.
// =============================================================================
#include "settings.h"
#include "logger.h"

#include <windhawk_api.h>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <functional>

namespace TaskbarCustomizer {

// ─── Private helpers ──────────────────────────────────────────────────────────

double SettingsManager::ReadDouble(const wchar_t* key, double def) noexcept {
    // Windhawk stores everything as strings; we parse manually for maximum
    // resilience. On any failure we fall back to the supplied default.
    auto raw = Wh_GetStringSetting(key);
    if (!raw || raw[0] == L'\0') {
        Wh_FreeStringSetting(raw);
        return def;
    }
    wchar_t* end = nullptr;
    double val = std::wcstod(raw, &end);
    Wh_FreeStringSetting(raw);
    if (end == raw || std::isnan(val) || std::isinf(val)) {
        return def;
    }
    return val;
}

int SettingsManager::ReadInt(const wchar_t* key, int def) noexcept {
    return Wh_GetIntValue(key, def);
}

bool SettingsManager::ReadBool(const wchar_t* key, bool def) noexcept {
    return Wh_GetIntValue(key, def ? 1 : 0) != 0;
}

std::wstring SettingsManager::ReadString(const wchar_t* key,
                                          const wchar_t* def) noexcept {
    auto raw = Wh_GetStringSetting(key);
    std::wstring result = (raw && raw[0] != L'\0') ? raw : def;
    Wh_FreeStringSetting(raw);
    return result;
}

// ─── Load ─────────────────────────────────────────────────────────────────────

ModSettings SettingsManager::Load() noexcept {
    ModSettings s;

    // Button geometry
    s.buttonWidth           = ReadDouble(L"buttonWidth",           56.0);
    s.buttonHeight          = ReadDouble(L"buttonHeight",          48.0);
    s.buttonMinWidth        = ReadDouble(L"buttonMinWidth",        32.0);
    s.buttonMaxWidth        = ReadDouble(L"buttonMaxWidth",       160.0);
    s.cornerRadius          = ReadDouble(L"cornerRadius",           4.0);

    // Padding / Margin
    s.paddingHorizontal     = ReadDouble(L"paddingHorizontal",      8.0);
    s.paddingVertical       = ReadDouble(L"paddingVertical",        4.0);
    s.marginHorizontal      = ReadDouble(L"marginHorizontal",       1.0);
    s.marginVertical        = ReadDouble(L"marginVertical",         0.0);

    // Background / Border
    s.backgroundOpacity     = ReadDouble(L"backgroundOpacity",      1.0);
    s.borderThickness       = ReadDouble(L"borderThickness",        0.0);

    // Icon
    s.iconSize              = ReadDouble(L"iconSize",              24.0);
    s.iconOffsetX           = ReadDouble(L"iconOffsetX",            0.0);
    s.iconOffsetY           = ReadDouble(L"iconOffsetY",            0.0);
    s.iconOpacity           = ReadDouble(L"iconOpacity",            1.0);
    s.iconScale             = ReadDouble(L"iconScale",              1.0);

    // Running indicator
    s.indicatorHeight       = ReadDouble(L"indicatorHeight",        3.0);
    s.indicatorWidth        = ReadDouble(L"indicatorWidth",        16.0);
    s.indicatorCornerRadius = ReadDouble(L"indicatorCornerRadius",  2.0);

    // Hover
    s.hoverAnimationEnabled = ReadBool  (L"hoverAnimationEnabled",  true);
    s.hoverScale            = ReadDouble(L"hoverScale",             1.0);
    s.hoverOpacity          = ReadDouble(L"hoverOpacity",           1.0);
    s.hoverDurationMs       = ReadDouble(L"hoverDurationMs",      150.0);

    // Pressed
    s.pressedScale          = ReadDouble(L"pressedScale",           0.95);
    s.pressedOpacity        = ReadDouble(L"pressedOpacity",         0.9);
    s.pressedDurationMs     = ReadDouble(L"pressedDurationMs",     80.0);

    // Spacing
    s.spacingBetweenButtons = ReadDouble(L"spacingBetweenButtons",  2.0);

    // Profile
    s.profile               = ReadString(L"profile",              L"Custom");

    LOG_DEBUG(L"[Settings] Loaded: width=%.1f height=%.1f radius=%.1f "
              L"paddH=%.1f paddV=%.1f marginH=%.1f marginV=%.1f "
              L"iconSz=%.1f indH=%.1f indW=%.1f profile=%ls",
              s.buttonWidth, s.buttonHeight, s.cornerRadius,
              s.paddingHorizontal, s.paddingVertical,
              s.marginHorizontal, s.marginVertical,
              s.iconSize, s.indicatorHeight, s.indicatorWidth,
              s.profile.c_str());

    return s;
}

// ─── Validate ─────────────────────────────────────────────────────────────────

ValidationResult SettingsManager::Validate(const ModSettings& s) noexcept {
    // Helper lambdas for range checks
    auto checkRange = [](double v, double lo, double hi,
                         const wchar_t* name) -> ValidationResult {
        if (v < lo || v > hi) {
            wchar_t buf[256];
            _snwprintf_s(buf, _countof(buf), _TRUNCATE,
                L"[Settings] %ls=%.2f out of range [%.2f, %.2f]",
                name, v, lo, hi);
            return ValidationResult::Failure(buf);
        }
        return ValidationResult::Success();
    };

    // Button geometry
    if (auto r = checkRange(s.buttonWidth,      0, 400, L"buttonWidth");    !r.ok) return r;
    if (auto r = checkRange(s.buttonHeight,     0, 200, L"buttonHeight");   !r.ok) return r;
    if (auto r = checkRange(s.buttonMinWidth,   0, 400, L"buttonMinWidth"); !r.ok) return r;
    if (auto r = checkRange(s.buttonMaxWidth,   0, 800, L"buttonMaxWidth"); !r.ok) return r;
    if (auto r = checkRange(s.cornerRadius,     0, 100, L"cornerRadius");   !r.ok) return r;

    // Cross-field: min <= max
    if (s.buttonMinWidth > s.buttonMaxWidth) {
        return ValidationResult::Failure(
            L"[Settings] buttonMinWidth must not exceed buttonMaxWidth");
    }

    // Padding / Margin
    if (auto r = checkRange(s.paddingHorizontal, -200, 200, L"paddingHorizontal"); !r.ok) return r;
    if (auto r = checkRange(s.paddingVertical,   -200, 200, L"paddingVertical");   !r.ok) return r;
    if (auto r = checkRange(s.marginHorizontal,  -200, 200, L"marginHorizontal");  !r.ok) return r;
    if (auto r = checkRange(s.marginVertical,    -200, 200, L"marginVertical");    !r.ok) return r;

    // Opacity (0..1)
    if (auto r = checkRange(s.backgroundOpacity, 0, 1, L"backgroundOpacity"); !r.ok) return r;
    if (auto r = checkRange(s.iconOpacity,       0, 1, L"iconOpacity");       !r.ok) return r;
    if (auto r = checkRange(s.hoverOpacity,      0, 1, L"hoverOpacity");      !r.ok) return r;
    if (auto r = checkRange(s.pressedOpacity,    0, 1, L"pressedOpacity");    !r.ok) return r;

    // Icon
    if (auto r = checkRange(s.iconSize,    4, 128, L"iconSize");   !r.ok) return r;
    if (auto r = checkRange(s.iconScale,   0.1, 5, L"iconScale");  !r.ok) return r;

    // Indicator
    if (auto r = checkRange(s.indicatorHeight,       0, 80, L"indicatorHeight");       !r.ok) return r;
    if (auto r = checkRange(s.indicatorWidth,        0, 200,L"indicatorWidth");        !r.ok) return r;
    if (auto r = checkRange(s.indicatorCornerRadius, 0, 40, L"indicatorCornerRadius"); !r.ok) return r;

    // Animation scales/durations
    if (auto r = checkRange(s.hoverScale,       0.5,  5,    L"hoverScale");       !r.ok) return r;
    if (auto r = checkRange(s.hoverDurationMs,  0,    2000, L"hoverDurationMs");  !r.ok) return r;
    if (auto r = checkRange(s.pressedScale,     0.5,  2,    L"pressedScale");     !r.ok) return r;
    if (auto r = checkRange(s.pressedDurationMs,0,    2000, L"pressedDurationMs");!r.ok) return r;

    // Spacing
    if (auto r = checkRange(s.spacingBetweenButtons, 0, 50, L"spacingBetweenButtons"); !r.ok) return r;

    return ValidationResult::Success();
}

// ─── LoadAndValidate ──────────────────────────────────────────────────────────

ModSettings SettingsManager::LoadAndValidate(ValidationResult& outResult) noexcept {
    ModSettings s = Load();
    outResult = Validate(s);
    if (!outResult.ok) {
        LOG_WARN(L"%ls", outResult.message.c_str());
    }
    return s;
}

// ─── ApplyProfile ─────────────────────────────────────────────────────────────

void SettingsManager::ApplyProfile(const std::wstring& profileName,
                                    ModSettings& s) noexcept {
    // Each profile overrides a specific subset of fields; all others retain
    // their user values (or defaults).

    if (profileName == L"Compact") {
        s.buttonWidth           = 40.0;
        s.buttonHeight          = 40.0;
        s.cornerRadius          = 4.0;
        s.paddingHorizontal     = 4.0;
        s.paddingVertical       = 2.0;
        s.iconSize              = 20.0;
        s.spacingBetweenButtons = 1.0;
        s.indicatorHeight       = 2.0;
        s.indicatorWidth        = 12.0;
        LOG_INFO(L"[Settings] Applied profile: Compact");
        return;
    }

    if (profileName == L"Large") {
        s.buttonWidth           = 72.0;
        s.buttonHeight          = 56.0;
        s.cornerRadius          = 8.0;
        s.paddingHorizontal     = 12.0;
        s.paddingVertical       = 8.0;
        s.iconSize              = 32.0;
        s.spacingBetweenButtons = 4.0;
        s.indicatorHeight       = 4.0;
        s.indicatorWidth        = 24.0;
        LOG_INFO(L"[Settings] Applied profile: Large");
        return;
    }

    if (profileName == L"Minimal") {
        s.buttonWidth           = 44.0;
        s.buttonHeight          = 44.0;
        s.cornerRadius          = 0.0;
        s.paddingHorizontal     = 4.0;
        s.paddingVertical       = 4.0;
        s.borderThickness       = 0.0;
        s.backgroundOpacity     = 0.0;
        s.indicatorHeight       = 2.0;
        s.indicatorWidth        = 8.0;
        s.spacingBetweenButtons = 0.0;
        LOG_INFO(L"[Settings] Applied profile: Minimal");
        return;
    }

    if (profileName == L"Rounded") {
        s.cornerRadius          = 24.0;
        s.paddingHorizontal     = 10.0;
        s.indicatorCornerRadius = 3.0;
        LOG_INFO(L"[Settings] Applied profile: Rounded");
        return;
    }

    if (profileName == L"Gaming") {
        s.buttonWidth           = 60.0;
        s.buttonHeight          = 52.0;
        s.cornerRadius          = 6.0;
        s.paddingHorizontal     = 10.0;
        s.iconSize              = 28.0;
        s.indicatorHeight       = 3.0;
        s.indicatorWidth        = 20.0;
        s.hoverScale            = 1.08;
        s.hoverDurationMs       = 100.0;
        s.pressedScale          = 0.93;
        LOG_INFO(L"[Settings] Applied profile: Gaming");
        return;
    }

    if (profileName == L"Developer") {
        s.buttonWidth           = 56.0;
        s.buttonHeight          = 48.0;
        s.cornerRadius          = 2.0;
        s.paddingHorizontal     = 6.0;
        s.spacingBetweenButtons = 2.0;
        LOG_INFO(L"[Settings] Applied profile: Developer");
        return;
    }

    if (profileName == L"Tablet") {
        s.buttonWidth           = 80.0;
        s.buttonHeight          = 64.0;
        s.cornerRadius          = 12.0;
        s.paddingHorizontal     = 16.0;
        s.paddingVertical       = 12.0;
        s.iconSize              = 36.0;
        s.spacingBetweenButtons = 6.0;
        LOG_INFO(L"[Settings] Applied profile: Tablet");
        return;
    }

    // "Custom" or unknown — no overrides; user values are used as-is.
    LOG_DEBUG(L"[Settings] No profile override for '%ls'; using custom values.",
              profileName.c_str());
}

} // namespace TaskbarCustomizer
