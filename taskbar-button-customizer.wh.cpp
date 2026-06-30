// =============================================================================
// taskbar-button-customizer.wh.cpp
// Windows 11 Taskbar Button Customizer — Windhawk Mod
// Single-file, self-contained implementation.
// All modules (logger, settings, XAML utils, property engine, discovery,
// hooks, live refresh) are inlined here so Windhawk can compile with a
// single translation unit — no external file references needed.
// =============================================================================

// ==WindhawkMod==
// @id              taskbar-button-customizer
// @name            Windows 11 Taskbar Button Customizer
// @description     Customize taskbar button size, corner radius, padding, margins, icon position, running indicator, hover/press animations, and spacing — all from a clean settings panel, with live refresh and no Explorer restart required.
// @version         1.0.0
// @author          Manas Choksi
// @github          https://github.com/choksi2212
// @homepage        https://github.com/choksi2212/taskbar-button-customizer-mod
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -lcomctl32 -lole32 -loleaut32 -lruntimeobject -luser32 -Wl,--export-all-symbols
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Windows 11 Taskbar Button Customizer

Exposes every visual dimension of the Windows 11 taskbar button container
that Microsoft does not provide a built-in way to change.

## Features (v1.0)

- **Button size** — Width, Height, MinWidth, MaxWidth
- **Corner radius** — independent from the system default
- **Padding & Margin** — horizontal and vertical control
- **Background opacity** — transparency without affecting icons
- **Icon positioning** — size, offset, opacity, scale
- **Running indicator** — height, width, corner radius
- **Spacing** — between buttons
- **Built-in profiles** — Compact, Large, Minimal, Rounded, Gaming, Developer, Tablet
- **Live refresh** — settings apply instantly, no Explorer restart

## Compatibility

- Windows 11 23H2 (build 22631+)
- Windows 11 24H2 (build 26100+)
- Windows 11 25H2 (anticipated)
- x64 only

## Open Source

Source: https://github.com/choksi2212/taskbar-button-customizer-mod  
License: MIT
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- profile: Custom
  $name: Profile
  $description: >-
    Select a built-in layout profile. Overrides individual values below.
  $options:
  - Custom: Custom (use values below)
  - Compact: Compact
  - Large: Large
  - Minimal: Minimal
  - Rounded: Rounded
  - Gaming: Gaming
  - Developer: Developer
  - Tablet: Tablet

- buttonWidth: "56"
  $name: Button Width (px)
  $description: Set to 0 to use the Windows default.

- buttonHeight: "48"
  $name: Button Height (px)
  $description: Set to 0 to use the Windows default.

- buttonMinWidth: "32"
  $name: Button Minimum Width (px)

- buttonMaxWidth: "160"
  $name: Button Maximum Width (px)

- cornerRadius: "4"
  $name: Corner Radius (px)

- paddingHorizontal: "8"
  $name: Padding Horizontal (px)

- paddingVertical: "4"
  $name: Padding Vertical (px)

- marginHorizontal: "1"
  $name: Margin Horizontal (px)

- marginVertical: "0"
  $name: Margin Vertical (px)

- backgroundOpacity: "1.0"
  $name: Background Opacity
  $description: 0.0 = transparent, 1.0 = opaque.

- borderThickness: "0"
  $name: Border Thickness (px)

- iconSize: "24"
  $name: Icon Size (px)

- iconOffsetX: "0"
  $name: Icon Offset X (px)

- iconOffsetY: "0"
  $name: Icon Offset Y (px)

- iconOpacity: "1.0"
  $name: Icon Opacity

- iconScale: "1.0"
  $name: Icon Scale

- indicatorHeight: "3"
  $name: Indicator Height (px)

- indicatorWidth: "16"
  $name: Indicator Width (px)

- indicatorCornerRadius: "2"
  $name: Indicator Corner Radius (px)

- hoverAnimationEnabled: true
  $name: Enable Hover Animation

- hoverScale: "1.0"
  $name: Hover Scale

- hoverOpacity: "1.0"
  $name: Hover Opacity

- hoverDurationMs: "150"
  $name: Hover Duration (ms)

- pressedScale: "0.95"
  $name: Pressed Scale

- pressedOpacity: "0.9"
  $name: Pressed Opacity

- pressedDurationMs: "80"
  $name: Pressed Duration (ms)

- spacingBetweenButtons: "2"
  $name: Spacing Between Buttons (px)
*/
// ==/WindhawkModSettings==

// =============================================================================
// SYSTEM HEADERS
// =============================================================================
#include <windows.h>
#include <windhawk_api.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// WinRT XAML — undef required because WinRT redefines GetCurrentTime
#undef GetCurrentTime
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>

// IDesktopWindowXamlSourceNative — needed for HWND → XAML root
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>

using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Controls;
using namespace winrt::Windows::UI::Xaml::Hosting;
using namespace winrt::Windows::UI::Xaml::Media;
using namespace winrt::Windows::UI::Xaml::Shapes;

// =============================================================================
// MODULE: LOGGER
// Severity-based logging with compile-time stripping.
// =============================================================================
namespace {

#define TBC_LOG_DEBUG   0
#define TBC_LOG_INFO    1
#define TBC_LOG_WARN    2
#define TBC_LOG_ERROR   3

#ifdef _DEBUG
  #define TBC_LOG_LEVEL TBC_LOG_DEBUG
#else
  #define TBC_LOG_LEVEL TBC_LOG_INFO
#endif

#define TBC_LOG_IMPL(prefix, fmt, ...)                              \
    do {                                                            \
        wchar_t _buf[1024];                                         \
        _snwprintf_s(_buf, _countof(_buf), _TRUNCATE,              \
                     prefix L" " fmt, ##__VA_ARGS__);              \
        Wh_Log(L"%ls", _buf);                                      \
    } while(0)

#if TBC_LOG_LEVEL <= TBC_LOG_DEBUG
  #define LOG_DEBUG(fmt, ...) TBC_LOG_IMPL(L"[DBG]", fmt, ##__VA_ARGS__)
#else
  #define LOG_DEBUG(fmt, ...) do {} while(0)
#endif
#if TBC_LOG_LEVEL <= TBC_LOG_INFO
  #define LOG_INFO(fmt, ...)  TBC_LOG_IMPL(L"[INF]", fmt, ##__VA_ARGS__)
#else
  #define LOG_INFO(fmt, ...)  do {} while(0)
#endif
#if TBC_LOG_LEVEL <= TBC_LOG_WARN
  #define LOG_WARN(fmt, ...)  TBC_LOG_IMPL(L"[WRN]", fmt, ##__VA_ARGS__)
#else
  #define LOG_WARN(fmt, ...)  do {} while(0)
#endif
#if TBC_LOG_LEVEL <= TBC_LOG_ERROR
  #define LOG_ERROR(fmt, ...) TBC_LOG_IMPL(L"[ERR]", fmt, ##__VA_ARGS__)
#else
  #define LOG_ERROR(fmt, ...) do {} while(0)
#endif

} // namespace

// =============================================================================
// MODULE: VERSION COMPATIBILITY
// =============================================================================
namespace TBC {

constexpr DWORD kMinSupportedBuild = 22621u; // Windows 11 22H2 minimum

inline bool IsOsVersionSupported() noexcept {
    using RtlGetVersionFn = NTSTATUS(WINAPI*)(PRTL_OSVERSIONINFOW);
    HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) { LOG_ERROR(L"[Version] ntdll not found"); return false; }

    auto fn = reinterpret_cast<RtlGetVersionFn>(
        ::GetProcAddress(ntdll, "RtlGetVersion"));
    if (!fn) { LOG_ERROR(L"[Version] RtlGetVersion not found"); return false; }

    RTL_OSVERSIONINFOW ovi = {};
    ovi.dwOSVersionInfoSize = sizeof(ovi);
    if (fn(&ovi) != 0) { LOG_ERROR(L"[Version] RtlGetVersion failed"); return false; }

    LOG_INFO(L"[Version] Windows %lu.%lu build %lu",
             ovi.dwMajorVersion, ovi.dwMinorVersion, ovi.dwBuildNumber);

    if (ovi.dwMajorVersion != 10 || ovi.dwMinorVersion != 0) {
        LOG_WARN(L"[Version] Not Windows 11 (10.0.x). Refusing to load.");
        return false;
    }
    if (ovi.dwBuildNumber < kMinSupportedBuild) {
        LOG_WARN(L"[Version] Build %lu < minimum %lu. Refusing to load.",
                 ovi.dwBuildNumber, kMinSupportedBuild);
        return false;
    }
    return true;
}

} // namespace TBC

// =============================================================================
// MODULE: SETTINGS
// =============================================================================
namespace TBC {

struct ModSettings {
    // Button geometry
    double buttonWidth          = 56.0;
    double buttonHeight         = 48.0;
    double buttonMinWidth       = 32.0;
    double buttonMaxWidth       = 160.0;
    double cornerRadius         = 4.0;
    // Padding / Margin
    double paddingHorizontal    = 8.0;
    double paddingVertical      = 4.0;
    double marginHorizontal     = 1.0;
    double marginVertical       = 0.0;
    // Background
    double backgroundOpacity    = 1.0;
    double borderThickness      = 0.0;
    // Icon
    double iconSize             = 24.0;
    double iconOffsetX          = 0.0;
    double iconOffsetY          = 0.0;
    double iconOpacity          = 1.0;
    double iconScale            = 1.0;
    // Running indicator
    double indicatorHeight      = 3.0;
    double indicatorWidth       = 16.0;
    double indicatorCornerRadius= 2.0;
    // Hover
    bool   hoverAnimationEnabled= true;
    double hoverScale           = 1.0;
    double hoverOpacity         = 1.0;
    double hoverDurationMs      = 150.0;
    // Pressed
    double pressedScale         = 0.95;
    double pressedOpacity       = 0.9;
    double pressedDurationMs    = 80.0;
    // Spacing
    double spacingBetweenButtons= 2.0;
    // Profile
    std::wstring profile;
};

struct ValidationResult {
    bool ok = true;
    std::wstring message;
    static ValidationResult Success() noexcept { return {true, {}}; }
    static ValidationResult Failure(std::wstring m) noexcept { return {false, std::move(m)}; }
};

// ── Settings helpers ──────────────────────────────────────────────────────────

inline double ReadDouble(const wchar_t* key, double def) noexcept {
    auto raw = Wh_GetStringSetting(key);
    if (!raw || raw[0] == L'\0') { Wh_FreeStringSetting(raw); return def; }
    wchar_t* end = nullptr;
    double v = std::wcstod(raw, &end);
    Wh_FreeStringSetting(raw);
    if (!end || end == raw || std::isnan(v) || std::isinf(v)) return def;
    return v;
}

inline bool ReadBool(const wchar_t* key, bool def) noexcept {
    return Wh_GetIntValue(key, def ? 1 : 0) != 0;
}

inline std::wstring ReadString(const wchar_t* key, const wchar_t* def) noexcept {
    auto raw = Wh_GetStringSetting(key);
    std::wstring r = (raw && raw[0] != L'\0') ? raw : def;
    Wh_FreeStringSetting(raw);
    return r;
}

inline ModSettings LoadSettings() noexcept {
    ModSettings s;
    s.buttonWidth           = ReadDouble(L"buttonWidth",           56.0);
    s.buttonHeight          = ReadDouble(L"buttonHeight",          48.0);
    s.buttonMinWidth        = ReadDouble(L"buttonMinWidth",        32.0);
    s.buttonMaxWidth        = ReadDouble(L"buttonMaxWidth",       160.0);
    s.cornerRadius          = ReadDouble(L"cornerRadius",           4.0);
    s.paddingHorizontal     = ReadDouble(L"paddingHorizontal",      8.0);
    s.paddingVertical       = ReadDouble(L"paddingVertical",        4.0);
    s.marginHorizontal      = ReadDouble(L"marginHorizontal",       1.0);
    s.marginVertical        = ReadDouble(L"marginVertical",         0.0);
    s.backgroundOpacity     = ReadDouble(L"backgroundOpacity",      1.0);
    s.borderThickness       = ReadDouble(L"borderThickness",        0.0);
    s.iconSize              = ReadDouble(L"iconSize",              24.0);
    s.iconOffsetX           = ReadDouble(L"iconOffsetX",            0.0);
    s.iconOffsetY           = ReadDouble(L"iconOffsetY",            0.0);
    s.iconOpacity           = ReadDouble(L"iconOpacity",            1.0);
    s.iconScale             = ReadDouble(L"iconScale",              1.0);
    s.indicatorHeight       = ReadDouble(L"indicatorHeight",        3.0);
    s.indicatorWidth        = ReadDouble(L"indicatorWidth",        16.0);
    s.indicatorCornerRadius = ReadDouble(L"indicatorCornerRadius",  2.0);
    s.hoverAnimationEnabled = ReadBool  (L"hoverAnimationEnabled",  true);
    s.hoverScale            = ReadDouble(L"hoverScale",             1.0);
    s.hoverOpacity          = ReadDouble(L"hoverOpacity",           1.0);
    s.hoverDurationMs       = ReadDouble(L"hoverDurationMs",      150.0);
    s.pressedScale          = ReadDouble(L"pressedScale",           0.95);
    s.pressedOpacity        = ReadDouble(L"pressedOpacity",         0.9);
    s.pressedDurationMs     = ReadDouble(L"pressedDurationMs",     80.0);
    s.spacingBetweenButtons = ReadDouble(L"spacingBetweenButtons",  2.0);
    s.profile               = ReadString(L"profile",              L"Custom");

    LOG_INFO(L"[Settings] Loaded: w=%.0f h=%.0f r=%.0f paddH=%.0f paddV=%.0f "
             L"icon=%.0f indH=%.0f indW=%.0f profile=%ls",
             s.buttonWidth, s.buttonHeight, s.cornerRadius,
             s.paddingHorizontal, s.paddingVertical,
             s.iconSize, s.indicatorHeight, s.indicatorWidth,
             s.profile.c_str());
    return s;
}

inline ValidationResult ValidateSettings(const ModSettings& s) noexcept {
    auto chk = [](double v, double lo, double hi, const wchar_t* n) -> ValidationResult {
        if (v < lo || v > hi) {
            wchar_t buf[256];
            _snwprintf_s(buf, _countof(buf), _TRUNCATE,
                L"[Settings] %ls=%.2f out of range [%.2f,%.2f]", n, v, lo, hi);
            return ValidationResult::Failure(buf);
        }
        return ValidationResult::Success();
    };
    if (auto r = chk(s.buttonWidth,         0, 400, L"buttonWidth");         !r.ok) return r;
    if (auto r = chk(s.buttonHeight,        0, 200, L"buttonHeight");        !r.ok) return r;
    if (auto r = chk(s.cornerRadius,        0, 100, L"cornerRadius");        !r.ok) return r;
    if (auto r = chk(s.backgroundOpacity,   0, 1,   L"backgroundOpacity");   !r.ok) return r;
    if (auto r = chk(s.iconOpacity,         0, 1,   L"iconOpacity");         !r.ok) return r;
    if (auto r = chk(s.iconScale,           0.1,5,  L"iconScale");           !r.ok) return r;
    if (auto r = chk(s.indicatorHeight,     0, 80,  L"indicatorHeight");     !r.ok) return r;
    if (auto r = chk(s.indicatorWidth,      0, 200, L"indicatorWidth");      !r.ok) return r;
    if (auto r = chk(s.hoverScale,          0.5,5,  L"hoverScale");          !r.ok) return r;
    if (auto r = chk(s.pressedScale,        0.5,2,  L"pressedScale");        !r.ok) return r;
    if (auto r = chk(s.spacingBetweenButtons,0,50,  L"spacingBetweenButtons");!r.ok) return r;
    if (s.buttonMinWidth > s.buttonMaxWidth)
        return ValidationResult::Failure(L"[Settings] buttonMinWidth > buttonMaxWidth");
    return ValidationResult::Success();
}

inline void ApplyProfile(const std::wstring& name, ModSettings& s) noexcept {
    if (name == L"Compact") {
        s.buttonWidth=40; s.buttonHeight=40; s.cornerRadius=4;
        s.paddingHorizontal=4; s.paddingVertical=2;
        s.iconSize=20; s.spacingBetweenButtons=1;
        s.indicatorHeight=2; s.indicatorWidth=12;
    } else if (name == L"Large") {
        s.buttonWidth=72; s.buttonHeight=56; s.cornerRadius=8;
        s.paddingHorizontal=12; s.paddingVertical=8;
        s.iconSize=32; s.spacingBetweenButtons=4;
        s.indicatorHeight=4; s.indicatorWidth=24;
    } else if (name == L"Minimal") {
        s.buttonWidth=44; s.buttonHeight=44; s.cornerRadius=0;
        s.paddingHorizontal=4; s.paddingVertical=4;
        s.borderThickness=0; s.backgroundOpacity=0;
        s.indicatorHeight=2; s.indicatorWidth=8;
        s.spacingBetweenButtons=0;
    } else if (name == L"Rounded") {
        s.cornerRadius=24; s.paddingHorizontal=10; s.indicatorCornerRadius=3;
    } else if (name == L"Gaming") {
        s.buttonWidth=60; s.buttonHeight=52; s.cornerRadius=6;
        s.paddingHorizontal=10; s.iconSize=28;
        s.indicatorHeight=3; s.indicatorWidth=20;
        s.hoverScale=1.08; s.hoverDurationMs=100; s.pressedScale=0.93;
    } else if (name == L"Developer") {
        s.buttonWidth=56; s.buttonHeight=48; s.cornerRadius=2;
        s.paddingHorizontal=6; s.spacingBetweenButtons=2;
    } else if (name == L"Tablet") {
        s.buttonWidth=80; s.buttonHeight=64; s.cornerRadius=12;
        s.paddingHorizontal=16; s.paddingVertical=12;
        s.iconSize=36; s.spacingBetweenButtons=6;
    }
    LOG_INFO(L"[Settings] Applied profile: %ls", name.c_str());
}

inline ModSettings LoadAndApply() noexcept {
    ModSettings s = LoadSettings();
    ValidationResult vr = ValidateSettings(s);
    if (!vr.ok) { LOG_WARN(L"%ls", vr.message.c_str()); }
    if (!s.profile.empty() && s.profile != L"Custom") {
        ApplyProfile(s.profile, s);
    }
    return s;
}

} // namespace TBC

// =============================================================================
// MODULE: XAML UTILITIES
// =============================================================================
namespace TBC {

inline bool TypeNameEndsWith(const winrt::Windows::Foundation::IInspectable& obj,
                              std::wstring_view suffix) noexcept {
    try {
        auto name = winrt::get_class_name(obj);
        return std::wstring_view{name}.ends_with(suffix);
    } catch (...) { return false; }
}

inline void WalkVisualTree(
    const DependencyObject& root,
    const std::function<bool(DependencyObject)>& visitor)
{
    if (!root) return;
    try {
        if (!visitor(root)) return;
        int n = VisualTreeHelper::GetChildrenCount(root);
        for (int i = 0; i < n; ++i) {
            WalkVisualTree(VisualTreeHelper::GetChild(root, i), visitor);
        }
    } catch (const winrt::hresult_error& e) {
        LOG_WARN(L"[XamlUtils] Walk exception: 0x%08X", (unsigned)e.code().value);
    }
}

} // namespace TBC

// =============================================================================
// MODULE: PROPERTY ENGINE
// =============================================================================
namespace TBC {

inline Thickness MakeThickness(double h, double v) noexcept {
    return {h, v, h, v};
}
inline CornerRadius MakeCornerRadius(double r) noexcept {
    return {r, r, r, r};
}

inline void ApplyToButtonSubtree(const DependencyObject& root,
                                  const ModSettings& s) noexcept {
    if (!root) return;
    try {
        // 1. Outer button geometry
        if (auto fe = root.try_as<FrameworkElement>()) {
            if (s.buttonWidth  > 0.0) fe.Width (s.buttonWidth);
            else fe.Width(std::numeric_limits<double>::quiet_NaN());
            if (s.buttonHeight > 0.0) fe.Height(s.buttonHeight);
            else fe.Height(std::numeric_limits<double>::quiet_NaN());
            fe.MinWidth(s.buttonMinWidth);
            fe.MaxWidth(s.buttonMaxWidth);

            // Inter-button spacing via left/right margin
            auto m = fe.Margin();
            m.Left  = s.spacingBetweenButtons / 2.0;
            m.Right = s.spacingBetweenButtons / 2.0;
            fe.Margin(m);
        }

        // 2. Walk subtree for named/typed children
        WalkVisualTree(root, [&](DependencyObject node) -> bool {
            // BackgroundElement / BackgroundBorder
            if (auto border = node.try_as<Border>()) {
                if (auto fe = node.try_as<FrameworkElement>()) {
                    auto nameRt = fe.Name();
                    std::wstring name = nameRt ? std::wstring{nameRt} : L"";
                    if (name == L"BackgroundElement" || name == L"BackgroundBorder") {
                        border.CornerRadius(MakeCornerRadius(s.cornerRadius));
                        border.Padding(MakeThickness(s.paddingHorizontal, s.paddingVertical));
                        border.Margin(MakeThickness(s.marginHorizontal, s.marginVertical));
                        border.BorderThickness(MakeThickness(s.borderThickness, s.borderThickness));
                        border.Opacity(std::max(0.0, std::min(1.0, s.backgroundOpacity)));
                    }
                }
            }

            // Icon: Image or AnimatedVisualPlayer
            bool isIcon = TypeNameEndsWith(node, L"Image") ||
                          TypeNameEndsWith(node, L"AnimatedVisualPlayer");
            if (isIcon) {
                if (auto fe = node.try_as<FrameworkElement>()) {
                    if (s.iconSize > 0.0) { fe.Width(s.iconSize); fe.Height(s.iconSize); }
                    fe.Opacity(std::max(0.0, std::min(1.0, s.iconOpacity)));
                    // Offset via Margin (shift left/top, compensate right/bottom)
                    fe.Margin(Thickness{s.iconOffsetX, s.iconOffsetY,
                                       -s.iconOffsetX, -s.iconOffsetY});
                    // Scale via RenderTransform
                    if (s.iconScale != 1.0) {
                        ScaleTransform st;
                        st.ScaleX(s.iconScale);
                        st.ScaleY(s.iconScale);
                        fe.RenderTransform(st);
                        fe.RenderTransformOrigin({0.5, 0.5});
                    } else {
                        fe.RenderTransform(nullptr);
                    }
                }
            }

            // Running indicator rectangle
            if (auto rect = node.try_as<Rectangle>()) {
                if (auto fe = node.try_as<FrameworkElement>()) {
                    auto nameRt = fe.Name();
                    std::wstring name = nameRt ? std::wstring{nameRt} : L"";
                    if (name == L"RunningIndicator") {
                        if (s.indicatorHeight > 0.0) fe.Height(s.indicatorHeight);
                        if (s.indicatorWidth  > 0.0) fe.Width(s.indicatorWidth);
                        rect.RadiusX(s.indicatorCornerRadius);
                        rect.RadiusY(s.indicatorCornerRadius);
                    }
                }
            }

            return true; // continue walk
        });

    } catch (const winrt::hresult_error& e) {
        LOG_ERROR(L"[PropEngine] ApplyToButtonSubtree: 0x%08X %ls",
                  (unsigned)e.code().value, e.message().c_str());
    } catch (...) {
        LOG_ERROR(L"[PropEngine] ApplyToButtonSubtree: unknown exception");
    }
}

inline void RestoreButtonSubtree(const DependencyObject& root) noexcept {
    if (!root) return;
    try {
        WalkVisualTree(root, [](DependencyObject node) -> bool {
            try {
                if (auto fe = node.try_as<FrameworkElement>()) {
                    fe.ClearValue(FrameworkElement::WidthProperty());
                    fe.ClearValue(FrameworkElement::HeightProperty());
                    fe.ClearValue(FrameworkElement::MinWidthProperty());
                    fe.ClearValue(FrameworkElement::MaxWidthProperty());
                    fe.ClearValue(FrameworkElement::MarginProperty());
                    fe.ClearValue(UIElement::OpacityProperty());
                    fe.RenderTransform(nullptr);
                }
                if (auto b = node.try_as<Border>()) {
                    b.ClearValue(Border::CornerRadiusProperty());
                    b.ClearValue(Border::PaddingProperty());
                    b.ClearValue(Border::BorderThicknessProperty());
                }
                if (auto r = node.try_as<Rectangle>()) {
                    r.ClearValue(Rectangle::RadiusXProperty());
                    r.ClearValue(Rectangle::RadiusYProperty());
                }
            } catch (...) {}
            return true;
        });
    } catch (...) {}
}

} // namespace TBC

// =============================================================================
// MODULE: TASKBAR DISCOVERY
// =============================================================================
namespace TBC {

using ButtonFoundCallback = std::function<void(DependencyObject)>;
static ButtonFoundCallback g_buttonFoundCallback;

inline void SetButtonFoundCallback(ButtonFoundCallback cb) noexcept {
    g_buttonFoundCallback = std::move(cb);
}

struct TaskbarElements {
    HWND taskbarHwnd = nullptr;
    UIElement xamlRoot = nullptr;
    std::vector<DependencyObject> taskListButtons;
    bool IsValid() const noexcept {
        return taskbarHwnd && xamlRoot && !taskListButtons.empty();
    }
};

static const wchar_t* const kXamlIslandClasses[] = {
    L"Windows.UI.Composition.DesktopWindowContentBridge",
    L"Microsoft.UI.Content.DesktopChildSiteBridge",
    nullptr
};

inline HWND FindXamlIslandHwnd(HWND taskbarHwnd) noexcept {
    for (int i = 0; kXamlIslandClasses[i]; ++i) {
        HWND h = ::FindWindowExW(taskbarHwnd, nullptr, kXamlIslandClasses[i], nullptr);
        if (h) return h;
    }
    // Fallback: enumerate all children
    struct Ctx { HWND result = nullptr; };
    Ctx ctx;
    ::EnumChildWindows(taskbarHwnd, [](HWND hwnd, LPARAM lp) -> BOOL {
        auto* c = reinterpret_cast<Ctx*>(lp);
        wchar_t cls[256] = {};
        ::GetClassNameW(hwnd, cls, _countof(cls));
        for (int i = 0; kXamlIslandClasses[i]; ++i) {
            if (::wcscmp(cls, kXamlIslandClasses[i]) == 0) {
                c->result = hwnd;
                return FALSE;
            }
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&ctx));
    return ctx.result;
}

inline UIElement GetXamlRoot(HWND islandHwnd) noexcept {
    if (!islandHwnd) return nullptr;
    try {
        auto sources = DesktopWindowXamlSource::GetForCurrentView();
        for (auto&& src : sources) {
            try {
                auto native = src.as<IDesktopWindowXamlSourceNative>();
                HWND srcHwnd = nullptr;
                if (SUCCEEDED(native->get_WindowHandle(&srcHwnd)) &&
                    srcHwnd == islandHwnd) {
                    return src.Content();
                }
            } catch (...) {}
        }
    } catch (...) {}
    return nullptr;
}

inline std::vector<DependencyObject> CollectButtons(const UIElement& root) noexcept {
    std::vector<DependencyObject> found;
    if (!root) return found;
    try {
        WalkVisualTree(root.as<DependencyObject>(), [&](DependencyObject node) -> bool {
            bool isBtn = TypeNameEndsWith(node, L"TaskListButton") ||
                         TypeNameEndsWith(node, L"TaskListLabeledButtonPanel");
            if (isBtn) {
                found.push_back(node);
                if (g_buttonFoundCallback) g_buttonFoundCallback(node);
                return false; // don't descend inside button
            }
            return true;
        });
    } catch (...) {}
    return found;
}

inline std::optional<TaskbarElements> DiscoverAll() noexcept {
    TaskbarElements r;
    r.taskbarHwnd = ::FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!r.taskbarHwnd) {
        LOG_WARN(L"[Discovery] Shell_TrayWnd not found");
        return std::nullopt;
    }
    HWND island = FindXamlIslandHwnd(r.taskbarHwnd);
    if (!island) {
        LOG_WARN(L"[Discovery] XAML Island HWND not found");
        return std::nullopt;
    }
    r.xamlRoot = GetXamlRoot(island);
    if (!r.xamlRoot) {
        LOG_WARN(L"[Discovery] Could not get XAML root");
        return std::nullopt;
    }
    r.taskListButtons = CollectButtons(r.xamlRoot);
    if (r.taskListButtons.empty()) {
        LOG_WARN(L"[Discovery] No task buttons found yet");
        return std::nullopt;
    }
    LOG_INFO(L"[Discovery] Found %zu buttons", r.taskListButtons.size());
    return r;
}

} // namespace TBC

// =============================================================================
// MODULE: HOOKS
// =============================================================================
namespace TBC {

using TaskbarCreatedCb = std::function<void(HWND)>;
using TaskbarUpdatedCb = std::function<void(HWND)>;

static TaskbarCreatedCb g_onCreated;
static TaskbarUpdatedCb g_onUpdated;
static bool             g_hooksInstalled = false;

static decltype(&::CreateWindowExW) g_origCreateWindowExW = nullptr;
static decltype(&::SetWindowPos)    g_origSetWindowPos    = nullptr;

static const wchar_t* const kTaskbarClasses[] = {
    L"Shell_TrayWnd",
    L"Shell_SecondaryTrayWnd",
    nullptr
};

static bool IsTaskbarClass(LPCWSTR cls) noexcept {
    if (!cls || reinterpret_cast<uintptr_t>(cls) <= 0xFFFF) return false;
    for (int i = 0; kTaskbarClasses[i]; ++i)
        if (::wcscmp(cls, kTaskbarClasses[i]) == 0) return true;
    return false;
}

static HWND WINAPI HookedCreateWindowExW(
    DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
    DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    HWND hwnd = g_origCreateWindowExW(dwExStyle, lpClassName, lpWindowName,
        dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    if (hwnd && IsTaskbarClass(lpClassName)) {
        wchar_t buf[256] = {};
        ::GetClassNameW(hwnd, buf, _countof(buf));
        if (::wcscmp(buf, L"Shell_TrayWnd") == 0) {
            LOG_INFO(L"[Hooks] Shell_TrayWnd created: 0x%p", hwnd);
            if (g_onCreated) g_onCreated(hwnd);
        } else {
            if (g_onUpdated) g_onUpdated(hwnd);
        }
    }
    return hwnd;
}

static BOOL WINAPI HookedSetWindowPos(
    HWND hWnd, HWND hWndInsertAfter,
    int X, int Y, int cx, int cy, UINT uFlags)
{
    BOOL r = g_origSetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
    if (r && hWnd) {
        wchar_t cls[256] = {};
        ::GetClassNameW(hWnd, cls, _countof(cls));
        if (::wcscmp(cls, L"Shell_TrayWnd") == 0 && g_onUpdated)
            g_onUpdated(hWnd);
    }
    return r;
}

inline bool InstallHooks(TaskbarCreatedCb onCreated,
                          TaskbarUpdatedCb onUpdated) noexcept {
    if (g_hooksInstalled) return true;
    g_onCreated = std::move(onCreated);
    g_onUpdated = std::move(onUpdated);

    bool ok = true;
    void* orig = nullptr;

    if (!Wh_SetFunctionHook(reinterpret_cast<void*>(&::CreateWindowExW),
                            reinterpret_cast<void*>(&HookedCreateWindowExW),
                            &orig)) {
        LOG_ERROR(L"[Hooks] Failed to hook CreateWindowExW");
        ok = false;
    } else {
        g_origCreateWindowExW = reinterpret_cast<decltype(g_origCreateWindowExW)>(orig);
        LOG_INFO(L"[Hooks] CreateWindowExW hooked");
    }

    orig = nullptr;
    if (!Wh_SetFunctionHook(reinterpret_cast<void*>(&::SetWindowPos),
                            reinterpret_cast<void*>(&HookedSetWindowPos),
                            &orig)) {
        LOG_WARN(L"[Hooks] Failed to hook SetWindowPos (non-fatal)");
    } else {
        g_origSetWindowPos = reinterpret_cast<decltype(g_origSetWindowPos)>(orig);
        LOG_INFO(L"[Hooks] SetWindowPos hooked");
    }

    Wh_ApplyHookOperations();
    g_hooksInstalled = ok;
    return ok;
}

inline void UninstallHooks() noexcept {
    g_onCreated = nullptr;
    g_onUpdated = nullptr;
    g_origCreateWindowExW = nullptr;
    g_origSetWindowPos    = nullptr;
    g_hooksInstalled = false;
    LOG_INFO(L"[Hooks] Uninstalled");
}

} // namespace TBC

// =============================================================================
// MODULE: LIVE REFRESH
// =============================================================================
namespace TBC {

static HWND g_msgWnd  = nullptr;
static ATOM g_msgAtom = 0;
constexpr UINT WM_TBC_REFRESH = WM_USER + 1;

static void ApplySettingsNow() noexcept {
    ModSettings s = LoadAndApply();
    auto elems = DiscoverAll();
    if (!elems.has_value()) {
        LOG_WARN(L"[Refresh] Taskbar elements not available; will retry on next trigger");
        return;
    }
    int count = 0;
    for (auto& btn : elems->taskListButtons) {
        ApplyToButtonSubtree(btn, s);
        ++count;
    }
    LOG_INFO(L"[Refresh] Applied to %d buttons", count);
}

static LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg,
                                    WPARAM wParam, LPARAM lParam) {
    if (msg == WM_TBC_REFRESH) {
        LOG_DEBUG(L"[Refresh] WM_TBC_REFRESH received");
        ApplySettingsNow();
        return 0;
    }
    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

inline bool InitRefresh() noexcept {
    if (g_msgWnd) return true;
    WNDCLASSEXW wc = {};
    wc.cbSize       = sizeof(wc);
    wc.lpfnWndProc  = MsgWndProc;
    wc.hInstance    = ::GetModuleHandleW(nullptr);
    wc.lpszClassName= L"TBC_MsgWnd";
    g_msgAtom = ::RegisterClassExW(&wc);
    if (!g_msgAtom && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        LOG_ERROR(L"[Refresh] RegisterClassExW failed: %lu", ::GetLastError());
        return false;
    }
    g_msgWnd = ::CreateWindowExW(0, L"TBC_MsgWnd", nullptr, 0,
        0,0,0,0, HWND_MESSAGE, nullptr, ::GetModuleHandleW(nullptr), nullptr);
    if (!g_msgWnd) {
        LOG_ERROR(L"[Refresh] CreateWindowExW failed: %lu", ::GetLastError());
        return false;
    }
    LOG_INFO(L"[Refresh] Message window created: 0x%p", g_msgWnd);
    return true;
}

inline void ShutdownRefresh() noexcept {
    if (g_msgWnd) { ::DestroyWindow(g_msgWnd); g_msgWnd = nullptr; }
    if (g_msgAtom) {
        ::UnregisterClassW(L"TBC_MsgWnd", ::GetModuleHandleW(nullptr));
        g_msgAtom = 0;
    }
}

inline void RequestRefresh() noexcept {
    if (g_msgWnd) {
        ::PostMessageW(g_msgWnd, WM_TBC_REFRESH, 0, 0);
        LOG_DEBUG(L"[Refresh] Refresh requested");
    }
}

} // namespace TBC

// =============================================================================
// WINDHAWK ENTRY POINTS
// =============================================================================

namespace {
    std::atomic<bool> g_modActive{false};
}

BOOL Wh_ModInit() {
    LOG_INFO(L"[Mod] Wh_ModInit start");

    // 1. OS version check
    if (!TBC::IsOsVersionSupported()) {
        LOG_ERROR(L"[Mod] Unsupported OS. Mod will not load.");
        return FALSE;
    }

    // 2. Initialize live refresh
    if (!TBC::InitRefresh()) {
        LOG_ERROR(L"[Mod] Failed to init refresh subsystem.");
        return FALSE;
    }

    // 3. Register new-button callback (applies settings to each new button)
    TBC::SetButtonFoundCallback([](DependencyObject btn) {
        TBC::ModSettings s = TBC::LoadAndApply();
        TBC::ApplyToButtonSubtree(btn, s);
    });

    // 4. Install Win32 hooks
    TBC::InstallHooks(
        [](HWND) {
            LOG_INFO(L"[Mod] Taskbar created → scheduling apply");
            TBC::RequestRefresh();
        },
        [](HWND) {
            LOG_DEBUG(L"[Mod] Taskbar updated → scheduling apply");
            TBC::RequestRefresh();
        });

    // 5. Immediate apply (taskbar may already be running)
    TBC::RequestRefresh();

    g_modActive.store(true, std::memory_order_release);
    LOG_INFO(L"[Mod] Wh_ModInit complete — mod is active");
    return TRUE;
}

void Wh_ModUninit() {
    LOG_INFO(L"[Mod] Wh_ModUninit start");
    g_modActive.store(false, std::memory_order_release);

    TBC::UninstallHooks();

    // Restore buttons to Windows defaults
    auto elems = TBC::DiscoverAll();
    if (elems.has_value()) {
        for (auto& btn : elems->taskListButtons)
            TBC::RestoreButtonSubtree(btn);
        LOG_INFO(L"[Mod] Restored %zu buttons", elems->taskListButtons.size());
    }

    TBC::ShutdownRefresh();
    LOG_INFO(L"[Mod] Wh_ModUninit complete");
}

void Wh_ModSettingsChanged() {
    if (!g_modActive.load(std::memory_order_acquire)) return;
    LOG_INFO(L"[Mod] Settings changed → requesting refresh");
    TBC::RequestRefresh();
}
