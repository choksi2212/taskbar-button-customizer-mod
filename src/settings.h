// =============================================================================
// settings.h — Taskbar Button Customizer
// Declares ModSettings structure and SettingsManager class.
// All user-facing configuration is centralised here; no other module should
// call Windhawk settings APIs directly.
// =============================================================================
#pragma once

#include <windows.h>
#include <string>
#include <optional>

namespace TaskbarCustomizer {

// ---------------------------------------------------------------------------
// ModSettings — Plain data struct carrying every configurable parameter.
// All numeric values use doubles internally so the UI can expose decimals.
// ---------------------------------------------------------------------------
struct ModSettings {
    // ── Button geometry ─────────────────────────────────────────────────────
    double buttonWidth          = 56.0;   // px; 0 = Windows default
    double buttonHeight         = 48.0;   // px; 0 = Windows default
    double buttonMinWidth       = 32.0;
    double buttonMaxWidth       = 160.0;
    double cornerRadius         = 4.0;    // px applied to BackgroundElement

    // ── Padding / Margin ─────────────────────────────────────────────────────
    double paddingHorizontal    = 8.0;
    double paddingVertical      = 4.0;
    double marginHorizontal     = 1.0;
    double marginVertical       = 0.0;

    // ── Background / Border ──────────────────────────────────────────────────
    double backgroundOpacity    = 1.0;    // 0.0–1.0
    double borderThickness      = 0.0;

    // ── Icon ─────────────────────────────────────────────────────────────────
    double iconSize             = 24.0;
    double iconOffsetX          = 0.0;
    double iconOffsetY          = 0.0;
    double iconOpacity          = 1.0;
    double iconScale            = 1.0;

    // ── Running indicator ────────────────────────────────────────────────────
    double indicatorHeight      = 3.0;
    double indicatorWidth       = 16.0;
    double indicatorCornerRadius= 2.0;

    // ── Hover state ──────────────────────────────────────────────────────────
    bool   hoverAnimationEnabled= true;
    double hoverScale           = 1.0;    // >1 = grow; typical 1.05–1.10
    double hoverOpacity         = 1.0;
    double hoverDurationMs      = 150.0;

    // ── Pressed state ───────────────────────────────────────────────────────
    double pressedScale         = 0.95;
    double pressedOpacity       = 0.9;
    double pressedDurationMs    = 80.0;

    // ── Spacing ──────────────────────────────────────────────────────────────
    double spacingBetweenButtons= 2.0;

    // ── Profile / preset ─────────────────────────────────────────────────────
    // Raw name from settings drop-down; empty → Custom
    std::wstring profile;
};

// ---------------------------------------------------------------------------
// ValidationResult — Carries success/failure together with a human-readable
// diagnostic message so callers can log precisely what went wrong.
// ---------------------------------------------------------------------------
struct ValidationResult {
    bool        ok      = true;
    std::wstring message;

    static ValidationResult Success() noexcept {
        return { true, L"" };
    }
    static ValidationResult Failure(std::wstring msg) noexcept {
        return { false, std::move(msg) };
    }
};

// ---------------------------------------------------------------------------
// SettingsManager — Stateless helper; all methods are static.
// Reads from Windhawk storage, validates, and returns an immutable snapshot.
// ---------------------------------------------------------------------------
class SettingsManager {
public:
    SettingsManager() = delete;

    /// Read every setting from Windhawk storage.
    /// Returns a fully-populated ModSettings with sane defaults for any
    /// key that is absent from storage.
    static ModSettings Load() noexcept;

    /// Validate all numeric ranges, cross-field constraints, etc.
    /// Returns Success() when the settings are safe to apply.
    static ValidationResult Validate(const ModSettings& s) noexcept;

    /// Convenience: load + validate in one call.
    /// On validation failure the returned settings still contain the
    /// loaded values (caller may choose to apply partial results).
    static ModSettings LoadAndValidate(ValidationResult& outResult) noexcept;

    /// Apply a built-in profile, overwriting the corresponding fields.
    static void ApplyProfile(const std::wstring& profileName,
                             ModSettings& inout) noexcept;

private:
    static double  ReadDouble(const wchar_t* key, double   def) noexcept;
    static int     ReadInt   (const wchar_t* key, int      def) noexcept;
    static bool    ReadBool  (const wchar_t* key, bool     def) noexcept;
    static std::wstring ReadString(const wchar_t* key,
                                   const wchar_t* def) noexcept;
};

} // namespace TaskbarCustomizer
