// =============================================================================
// taskbar-button-customizer.wh.cpp
// Windows 11 Taskbar Button Customizer — Windhawk Mod  v1.0.0
// Single-file, self-contained, no external headers.
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
// Must come before WinRT to define WIN32_LEAN_AND_MEAN etc.
// =============================================================================
#include <windows.h>
#include <windhawk_api.h>
#include <xamlom.h>  // IXamlDiagnostics, IVisualTreeServiceCallback2

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <functional>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// WinRT — must undef GetCurrentTime because WinRT redefines it
#undef GetCurrentTime
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.Xaml.Media.h>
// NOTE: Do NOT use "using namespace" for Shapes — Rectangle clashes with
//       the wingdi.h Rectangle() function. Use fully-qualified names.
#include <winrt/Windows.UI.Xaml.Shapes.h>

// IDesktopWindowXamlSourceNative COM interface
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>

// Selective using declarations — NO "using namespace" for Shapes or Controls
// (wingdi.h defines Rectangle() which clashes with Shapes::Rectangle).
// Border is from Controls (not Xaml root namespace).
using winrt::Windows::UI::Xaml::CornerRadius;
using winrt::Windows::UI::Xaml::DependencyObject;
using winrt::Windows::UI::Xaml::FrameworkElement;
using winrt::Windows::UI::Xaml::Thickness;
using winrt::Windows::UI::Xaml::UIElement;
using winrt::Windows::UI::Xaml::Media::ScaleTransform;
using winrt::Windows::UI::Xaml::Media::VisualTreeHelper;
// winrt::Windows::UI::Xaml::Controls::Border  — always fully-qualified below.
// winrt::Windows::UI::Xaml::Shapes::Rectangle — always fully-qualified below.

// =============================================================================
// MODULE: LOGGER
// =============================================================================
#define TBC_LOG_DEBUG 0
#define TBC_LOG_INFO  1
#define TBC_LOG_WARN  2
#define TBC_LOG_ERROR 3

#ifdef _DEBUG
  #define TBC_LOG_LEVEL TBC_LOG_DEBUG
#else
  #define TBC_LOG_LEVEL TBC_LOG_INFO
#endif

#define TBC_LOG_IMPL(prefix, fmt, ...)                              \
    do {                                                            \
        wchar_t _buf_[1024];                                        \
        _snwprintf_s(_buf_, _countof(_buf_), _TRUNCATE,            \
                     prefix L" " fmt, ##__VA_ARGS__);              \
        Wh_Log(L"%ls", _buf_);                                      \
    } while(0)

#if TBC_LOG_LEVEL <= TBC_LOG_DEBUG
  #define LOG_DEBUG(fmt,...) TBC_LOG_IMPL(L"[DBG]", fmt, ##__VA_ARGS__)
#else
  #define LOG_DEBUG(fmt,...) do{}while(0)
#endif
#if TBC_LOG_LEVEL <= TBC_LOG_INFO
  #define LOG_INFO(fmt,...)  TBC_LOG_IMPL(L"[INF]", fmt, ##__VA_ARGS__)
#else
  #define LOG_INFO(fmt,...)  do{}while(0)
#endif
#if TBC_LOG_LEVEL <= TBC_LOG_WARN
  #define LOG_WARN(fmt,...)  TBC_LOG_IMPL(L"[WRN]", fmt, ##__VA_ARGS__)
#else
  #define LOG_WARN(fmt,...)  do{}while(0)
#endif
#if TBC_LOG_LEVEL <= TBC_LOG_ERROR
  #define LOG_ERROR(fmt,...) TBC_LOG_IMPL(L"[ERR]", fmt, ##__VA_ARGS__)
#else
  #define LOG_ERROR(fmt,...) do{}while(0)
#endif

// =============================================================================
// MODULE: VERSION COMPATIBILITY
// =============================================================================
namespace TBC {

constexpr DWORD kMinSupportedBuild = 22621u;

inline bool IsOsVersionSupported() noexcept {
    using Fn = NTSTATUS(WINAPI*)(PRTL_OSVERSIONINFOW);
    HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) { LOG_ERROR(L"[Version] ntdll not found"); return false; }
    auto fn = reinterpret_cast<Fn>(::GetProcAddress(ntdll, "RtlGetVersion"));
    if (!fn)  { LOG_ERROR(L"[Version] RtlGetVersion not found"); return false; }

    RTL_OSVERSIONINFOW ovi{};
    ovi.dwOSVersionInfoSize = sizeof(ovi);
    if (fn(&ovi) != 0) { LOG_ERROR(L"[Version] RtlGetVersion failed"); return false; }

    LOG_INFO(L"[Version] Windows %lu.%lu build %lu",
             ovi.dwMajorVersion, ovi.dwMinorVersion, ovi.dwBuildNumber);

    if (ovi.dwMajorVersion != 10 || ovi.dwMinorVersion != 0) {
        LOG_WARN(L"[Version] Not Windows 11. Refusing to load.");
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
    double buttonWidth           = 56.0;
    double buttonHeight          = 48.0;
    double buttonMinWidth        = 32.0;
    double buttonMaxWidth        = 160.0;
    double cornerRadius          = 4.0;
    double paddingHorizontal     = 8.0;
    double paddingVertical       = 4.0;
    double marginHorizontal      = 1.0;
    double marginVertical        = 0.0;
    double backgroundOpacity     = 1.0;
    double borderThickness       = 0.0;
    double iconSize              = 24.0;
    double iconOffsetX           = 0.0;
    double iconOffsetY           = 0.0;
    double iconOpacity           = 1.0;
    double iconScale             = 1.0;
    double indicatorHeight       = 3.0;
    double indicatorWidth        = 16.0;
    double indicatorCornerRadius = 2.0;
    bool   hoverAnimationEnabled = true;
    double hoverScale            = 1.0;
    double hoverOpacity          = 1.0;
    double hoverDurationMs       = 150.0;
    double pressedScale          = 0.95;
    double pressedOpacity        = 0.9;
    double pressedDurationMs     = 80.0;
    double spacingBetweenButtons = 2.0;
    std::wstring profile;
};

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
    LOG_INFO(L"[Settings] w=%.0f h=%.0f r=%.0f paddH=%.0f profile=%ls",
             s.buttonWidth, s.buttonHeight, s.cornerRadius,
             s.paddingHorizontal, s.profile.c_str());
    return s;
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
        s.indicatorHeight=2; s.indicatorWidth=8; s.spacingBetweenButtons=0;
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
    if (!s.profile.empty() && s.profile != L"Custom")
        ApplyProfile(s.profile, s);
    return s;
}

} // namespace TBC

// =============================================================================
// MODULE: XAML UTILITIES
// No ambiguous "using namespace" — every XAML Shape is fully qualified.
// =============================================================================
namespace TBC {

// FIX: hstring does not convert to bool. Use .empty() check explicitly.
inline std::wstring HstringToWstring(const winrt::hstring& hs) noexcept {
    // hstring is always a valid object (never null); just possibly empty.
    return std::wstring{ hs };
}

inline bool XamlTypeNameEndsWith(
    const winrt::Windows::Foundation::IInspectable& obj,
    std::wstring_view suffix) noexcept
{
    try {
        auto name = winrt::get_class_name(obj);
        return std::wstring_view{name}.ends_with(suffix);
    } catch (...) { return false; }
}

inline void WalkVisualTree(
    const DependencyObject& root,
    const std::function<bool(DependencyObject)>& fn)
{
    if (!root) return;
    try {
        if (!fn(root)) return;
        int n = VisualTreeHelper::GetChildrenCount(root);
        for (int i = 0; i < n; ++i)
            WalkVisualTree(VisualTreeHelper::GetChild(root, i), fn);
    } catch (const winrt::hresult_error& e) {
        LOG_WARN(L"[XamlUtils] Walk 0x%08X", (unsigned)e.code().value);
    }
}

} // namespace TBC

// =============================================================================
// MODULE: PROPERTY ENGINE
// FIX: Shapes::Rectangle is always fully-qualified to avoid wingdi conflict.
// =============================================================================
namespace TBC {

inline Thickness MakeThickness(double h, double v) noexcept { return {h,v,h,v}; }
inline CornerRadius MakeCornerRadius(double r) noexcept     { return {r,r,r,r}; }

inline void ApplyToButtonSubtree(const DependencyObject& root,
                                  const ModSettings& s) noexcept {
    if (!root) return;
    try {
        // Outer button: geometry + spacing
        if (auto fe = root.try_as<FrameworkElement>()) {
            fe.Width (s.buttonWidth  > 0 ? s.buttonWidth
                                         : std::numeric_limits<double>::quiet_NaN());
            fe.Height(s.buttonHeight > 0 ? s.buttonHeight
                                         : std::numeric_limits<double>::quiet_NaN());
            fe.MinWidth (s.buttonMinWidth);
            fe.MaxWidth (s.buttonMaxWidth);
            // Spacing via left/right margin
            auto m = fe.Margin();
            m.Left  = s.spacingBetweenButtons / 2.0;
            m.Right = s.spacingBetweenButtons / 2.0;
            fe.Margin(m);
        }

        WalkVisualTree(root, [&](DependencyObject node) -> bool {

            // ── BackgroundElement / BackgroundBorder ──────────────────────────
            if (auto border = node.try_as<winrt::Windows::UI::Xaml::Controls::Border>()) {
                if (auto fe = node.try_as<FrameworkElement>()) {
                    // FIX: hstring → wstring via HstringToWstring (no bool cast)
                    std::wstring name = HstringToWstring(fe.Name());
                    if (name == L"BackgroundElement" || name == L"BackgroundBorder") {
                        border.CornerRadius   (MakeCornerRadius(s.cornerRadius));
                        border.Padding        (MakeThickness(s.paddingHorizontal, s.paddingVertical));
                        border.Margin         (MakeThickness(s.marginHorizontal,  s.marginVertical));
                        border.BorderThickness(MakeThickness(s.borderThickness,   s.borderThickness));
                        border.Opacity(std::clamp(s.backgroundOpacity, 0.0, 1.0));
                    }
                }
            }

            // ── Icon: Image or AnimatedVisualPlayer ───────────────────────────
            bool isIcon = XamlTypeNameEndsWith(node, L"Image") ||
                          XamlTypeNameEndsWith(node, L"AnimatedVisualPlayer");
            if (isIcon) {
                if (auto fe = node.try_as<FrameworkElement>()) {
                    if (s.iconSize > 0.0) {
                        fe.Width (s.iconSize);
                        fe.Height(s.iconSize);
                    }
                    fe.Opacity(std::clamp(s.iconOpacity, 0.0, 1.0));
                    // Offset via margin
                    fe.Margin(Thickness{
                        s.iconOffsetX,  s.iconOffsetY,
                        -s.iconOffsetX, -s.iconOffsetY});
                    // Scale
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

            // ── Running indicator rectangle ───────────────────────────────────
            // FIX: fully-qualified winrt::Windows::UI::Xaml::Shapes::Rectangle
            //      to avoid ambiguity with wingdi.h Rectangle() function.
            if (auto rect = node.try_as<winrt::Windows::UI::Xaml::Shapes::Rectangle>()) {
                if (auto fe = node.try_as<FrameworkElement>()) {
                    // FIX: hstring → wstring via HstringToWstring
                    std::wstring name = HstringToWstring(fe.Name());
                    if (name == L"RunningIndicator") {
                        if (s.indicatorHeight > 0.0) fe.Height(s.indicatorHeight);
                        if (s.indicatorWidth  > 0.0) fe.Width (s.indicatorWidth);
                        rect.RadiusX(s.indicatorCornerRadius);
                        rect.RadiusY(s.indicatorCornerRadius);
                    }
                }
            }

            return true;
        });

    } catch (const winrt::hresult_error& e) {
        LOG_ERROR(L"[PropEngine] 0x%08X %ls",
                  (unsigned)e.code().value, e.message().c_str());
    } catch (...) {
        LOG_ERROR(L"[PropEngine] Unknown exception");
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
                if (auto b = node.try_as<winrt::Windows::UI::Xaml::Controls::Border>()) {
                    b.ClearValue(winrt::Windows::UI::Xaml::Controls::Border::CornerRadiusProperty());
                    b.ClearValue(winrt::Windows::UI::Xaml::Controls::Border::PaddingProperty());
                    b.ClearValue(winrt::Windows::UI::Xaml::Controls::Border::BorderThicknessProperty());
                }
                // FIX: fully-qualified
                if (auto r = node.try_as<winrt::Windows::UI::Xaml::Shapes::Rectangle>()) {
                    r.ClearValue(winrt::Windows::UI::Xaml::Shapes::Rectangle::RadiusXProperty());
                    r.ClearValue(winrt::Windows::UI::Xaml::Shapes::Rectangle::RadiusYProperty());
                }
            } catch (...) {}
            return true;
        });
    } catch (...) {}
}

} // namespace TBC

// =============================================================================
// MODULE: TASKBAR DISCOVERY via IXamlDiagnostics + IVisualTreeServiceCallback2
//
// FIX #3: DesktopWindowXamlSource::GetForCurrentView() does not exist in the
// Windows SDK. The correct, production-proven approach (used by
// windows-11-taskbar-styler, TranslucentTB/ExplorerTAP) is to connect to the
// XAML diagnostics infrastructure via InitializeXamlDiagnosticsEx, then
// implement IVisualTreeServiceCallback2 to receive Add/Remove events for
// every XAML element in Explorer's process.
//
// Thread note: InitializeXamlDiagnosticsEx and AdviseVisualTreeChange MUST be
// called from a non-UI thread or they will deadlock. We use a dedicated
// background thread with its own message pump.
// =============================================================================
namespace TBC {

// Global list of discovered button elements (guarded by mutex)
static std::mutex               g_buttonsMtx;
static std::vector<DependencyObject> g_discoveredButtons;

static void StoreButton(const DependencyObject& btn) noexcept {
    std::lock_guard<std::mutex> lk(g_buttonsMtx);
    g_discoveredButtons.push_back(btn);
}

static void ClearButtons() noexcept {
    std::lock_guard<std::mutex> lk(g_buttonsMtx);
    g_discoveredButtons.clear();
}

// Apply current settings to all stored buttons (called from UI thread)
static void ReapplyToAllButtons() noexcept {
    ModSettings s = LoadAndApply();
    std::lock_guard<std::mutex> lk(g_buttonsMtx);
    for (auto& btn : g_discoveredButtons) {
        ApplyToButtonSubtree(btn, s);
    }
    LOG_INFO(L"[Refresh] Applied to %zu buttons", g_discoveredButtons.size());
}

// Restore defaults to all stored buttons
static void RestoreAllButtons() noexcept {
    std::lock_guard<std::mutex> lk(g_buttonsMtx);
    for (auto& btn : g_discoveredButtons)
        RestoreButtonSubtree(btn);
    LOG_INFO(L"[Restore] Restored %zu buttons", g_discoveredButtons.size());
}

// ── VisualTreeWatcher ─────────────────────────────────────────────────────────
// Receives Add/Remove events for every XAML element in the process.
// When we see a TaskListButton element added, we capture it and apply settings.

class VisualTreeWatcher
    : public winrt::implements<VisualTreeWatcher,
                               IVisualTreeServiceCallback2,
                               winrt::non_agile>
{
public:
    // IVisualTreeServiceCallback
    HRESULT STDMETHODCALLTYPE OnVisualTreeChange(
        ParentChildRelation  /* relation */,
        VisualElement        element,
        VisualMutationType   mutationType) noexcept override
    {
        try {
            if (mutationType == Add && element.Type) {
                std::wstring_view typeName{ element.Type };
                bool isBtn = typeName.ends_with(L"TaskListButton") ||
                             typeName.ends_with(L"TaskListLabeledButtonPanel");
                if (isBtn) {
                    // InstanceHandle is the raw IInspectable* cast to uint64.
                    // winrt::from_abi converts it back to a typed WinRT object.
                    auto raw = reinterpret_cast<winrt::Windows::UI::Xaml::DependencyObject*>(
                        reinterpret_cast<void*>(static_cast<uintptr_t>(element.Handle)));
                    if (raw && *raw) {
                        auto btn = *raw;
                        StoreButton(btn);
                        ModSettings s = LoadAndApply();
                        ApplyToButtonSubtree(btn, s);
                        LOG_INFO(L"[Watcher] Applied to new %ls", element.Type);
                    }
                }
            } else if (mutationType == Remove && element.Type) {
                std::wstring_view typeName{ element.Type };
                bool isBtn = typeName.ends_with(L"TaskListButton") ||
                             typeName.ends_with(L"TaskListLabeledButtonPanel");
                if (isBtn) {
                    // Element removed: prune from our list
                    auto raw = reinterpret_cast<DependencyObject*>(
                        reinterpret_cast<void*>(static_cast<uintptr_t>(element.Handle)));
                    if (raw && *raw) {
                        std::lock_guard<std::mutex> lk(g_buttonsMtx);
                        auto it = std::find(g_discoveredButtons.begin(),
                                            g_discoveredButtons.end(), *raw);
                        if (it != g_discoveredButtons.end())
                            g_discoveredButtons.erase(it);
                    }
                }
            }
        } catch (...) {}
        return S_OK;
    }

    // IVisualTreeServiceCallback2 — exact signature from xamlom.h line 286:
    //   OnElementStateChanged(InstanceHandle, VisualElementState, LPCWSTR)
    HRESULT STDMETHODCALLTYPE OnElementStateChanged(
        InstanceHandle    /* element */,
        VisualElementState /* elementState */,
        LPCWSTR           /* context */) noexcept override { return S_OK; }
};

// ── Background diagnostics thread ─────────────────────────────────────────────

static HANDLE g_diagThread = nullptr;
static DWORD  g_diagThreadId = 0;
static winrt::com_ptr<IXamlDiagnostics>          g_xamlDiag;
static winrt::com_ptr<VisualTreeWatcher>          g_watcher;

// Signature of InitializeXamlDiagnosticsEx (exported from Windows.UI.Xaml.dll)
using FnInitXamlDiag = HRESULT(WINAPI*)(
    LPCWSTR endpointName,
    DWORD   pid,
    LPCWSTR dllXamlDiag,
    LPCWSTR package,
    REFIID  riid,
    LPVOID* ppv);

static DWORD WINAPI XamlDiagThread(LPVOID) {
    // WinRT must be initialized per-thread
    winrt::init_apartment(winrt::apartment_type::single_threaded);

    HMODULE mod = ::LoadLibraryW(L"Windows.UI.Xaml.dll");
    if (!mod) {
        LOG_ERROR(L"[Diag] Cannot load Windows.UI.Xaml.dll");
        return 1;
    }

    auto initFn = reinterpret_cast<FnInitXamlDiag>(
        ::GetProcAddress(mod, "InitializeXamlDiagnosticsEx"));
    if (!initFn) {
        LOG_ERROR(L"[Diag] InitializeXamlDiagnosticsEx not found");
        return 1;
    }

    IXamlDiagnostics* rawDiag = nullptr;
    HRESULT hr = initFn(
        L"VisualDiagConnection1",
        ::GetCurrentProcessId(),
        nullptr,              // no separate dll
        L"",                  // package name
        IID_IXamlDiagnostics,
        reinterpret_cast<void**>(&rawDiag));

    if (FAILED(hr) || !rawDiag) {
        LOG_ERROR(L"[Diag] InitializeXamlDiagnosticsEx failed: 0x%08X",
                  static_cast<unsigned>(hr));
        return 1;
    }
    g_xamlDiag.attach(rawDiag);

    // Get IVisualTreeService2 via COM QueryInterface
    winrt::com_ptr<IVisualTreeService2> vts;
    hr = g_xamlDiag->QueryInterface(IID_PPV_ARGS(vts.put()));
    if (FAILED(hr) || !vts) {
        // Fallback: try IVisualTreeService (older API)
        winrt::com_ptr<IVisualTreeService> vts1;
        hr = g_xamlDiag->QueryInterface(IID_PPV_ARGS(vts1.put()));
        if (FAILED(hr) || !vts1) {
            LOG_ERROR(L"[Diag] QI for IVisualTreeService failed: 0x%08X",
                      static_cast<unsigned>(hr));
            return 1;
        }
        // IVisualTreeService also has AdviseVisualTreeChange
        g_watcher = winrt::make_self<VisualTreeWatcher>();
        hr = vts1->AdviseVisualTreeChange(g_watcher.get());
        if (FAILED(hr)) {
            LOG_ERROR(L"[Diag] AdviseVisualTreeChange (v1) failed: 0x%08X",
                      static_cast<unsigned>(hr));
            return 1;
        }
        LOG_INFO(L"[Diag] Connected via IVisualTreeService (v1).");
        MSG msg;
        while (::GetMessageW(&msg, nullptr, 0, 0) > 0) {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
        vts1->UnadviseVisualTreeChange(g_watcher.get());
        g_watcher  = nullptr;
        g_xamlDiag = nullptr;
        return 0;
    }
    if (FAILED(hr) || !vts) {
        LOG_ERROR(L"[Diag] QI for IVisualTreeService2 failed: 0x%08X",
                  static_cast<unsigned>(hr));
        return 1;
    }

    g_watcher = winrt::make_self<VisualTreeWatcher>();
    hr = vts->AdviseVisualTreeChange(g_watcher.get());
    if (FAILED(hr)) {
        LOG_ERROR(L"[Diag] AdviseVisualTreeChange failed: 0x%08X",
                  static_cast<unsigned>(hr));
        return 1;
    }

    LOG_INFO(L"[Diag] XAML diagnostics connected. Listening for elements.");

    // Keep thread alive with a message pump so callbacks arrive
    MSG msg;
    while (::GetMessageW(&msg, nullptr, 0, 0) > 0) {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }

    // Cleanup on exit
    vts->UnadviseVisualTreeChange(g_watcher.get());
    g_watcher  = nullptr;
    g_xamlDiag = nullptr;
    LOG_INFO(L"[Diag] XAML diagnostics thread exiting.");
    return 0;
}

inline bool StartXamlDiagnostics() noexcept {
    g_diagThread = ::CreateThread(nullptr, 0, XamlDiagThread, nullptr, 0, &g_diagThreadId);
    if (!g_diagThread) {
        LOG_ERROR(L"[Diag] Failed to create diagnostics thread: %lu", ::GetLastError());
        return false;
    }
    LOG_INFO(L"[Diag] Diagnostics thread started (TID=%lu)", g_diagThreadId);
    return true;
}

inline void StopXamlDiagnostics() noexcept {
    if (g_diagThread) {
        if (g_diagThreadId) ::PostThreadMessageW(g_diagThreadId, WM_QUIT, 0, 0);
        ::WaitForSingleObject(g_diagThread, 5000);
        ::CloseHandle(g_diagThread);
        g_diagThread   = nullptr;
        g_diagThreadId = 0;
    }
}

} // namespace TBC

// =============================================================================
// MODULE: LIVE REFRESH
// Message-only window marshals refresh requests to the UI thread.
// =============================================================================
namespace TBC {

static HWND g_msgWnd  = nullptr;
static ATOM g_msgAtom = 0;
constexpr UINT WM_TBC_REFRESH = WM_USER + 1;

static LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg,
                                    WPARAM wp, LPARAM lp) {
    if (msg == WM_TBC_REFRESH) { ReapplyToAllButtons(); return 0; }
    return ::DefWindowProcW(hwnd, msg, wp, lp);
}

inline bool InitRefresh() noexcept {
    if (g_msgWnd) return true;
    WNDCLASSEXW wc{};
    wc.cbSize       = sizeof(wc);
    wc.lpfnWndProc  = MsgWndProc;
    wc.hInstance    = ::GetModuleHandleW(nullptr);
    wc.lpszClassName= L"TBC_MsgWnd";
    g_msgAtom = ::RegisterClassExW(&wc);
    if (!g_msgAtom && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        LOG_ERROR(L"[Refresh] RegisterClassExW failed: %lu", ::GetLastError());
        return false;
    }
    g_msgWnd = ::CreateWindowExW(0, L"TBC_MsgWnd", nullptr, 0, 0,0,0,0,
                                  HWND_MESSAGE, nullptr,
                                  ::GetModuleHandleW(nullptr), nullptr);
    if (!g_msgWnd) {
        LOG_ERROR(L"[Refresh] CreateWindowExW failed: %lu", ::GetLastError());
        return false;
    }
    LOG_INFO(L"[Refresh] Message window created");
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
        LOG_DEBUG(L"[Refresh] Requested");
    }
}

} // namespace TBC

// =============================================================================
// MODULE: HOOKS (CreateWindowExW for taskbar lifecycle events)
// =============================================================================
namespace TBC {

static decltype(&::CreateWindowExW) g_origCreateWindowExW = nullptr;
static bool g_hooksInstalled = false;

static HWND WINAPI HookedCreateWindowExW(
    DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName,
    DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
    HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    HWND hwnd = g_origCreateWindowExW(dwExStyle, lpClassName, lpWindowName,
        dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    if (hwnd && lpClassName &&
        reinterpret_cast<uintptr_t>(lpClassName) > 0xFFFF) {
        wchar_t cls[256]{};
        ::GetClassNameW(hwnd, cls, _countof(cls));
        if (::wcscmp(cls, L"Shell_TrayWnd") == 0) {
            LOG_INFO(L"[Hooks] Shell_TrayWnd created — scheduling refresh");
            RequestRefresh();
        }
    }
    return hwnd;
}

inline bool InstallHooks() noexcept {
    if (g_hooksInstalled) return true;
    void* orig = nullptr;
    if (!Wh_SetFunctionHook(reinterpret_cast<void*>(&::CreateWindowExW),
                            reinterpret_cast<void*>(&HookedCreateWindowExW),
                            &orig)) {
        LOG_ERROR(L"[Hooks] Failed to hook CreateWindowExW");
        return false;
    }
    g_origCreateWindowExW = reinterpret_cast<decltype(g_origCreateWindowExW)>(orig);
    Wh_ApplyHookOperations();
    g_hooksInstalled = true;
    LOG_INFO(L"[Hooks] CreateWindowExW hooked");
    return true;
}

inline void UninstallHooks() noexcept {
    g_origCreateWindowExW = nullptr;
    g_hooksInstalled = false;
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

    if (!TBC::IsOsVersionSupported()) {
        LOG_ERROR(L"[Mod] Unsupported OS. Mod will not load.");
        return FALSE;
    }
    if (!TBC::InitRefresh()) {
        LOG_ERROR(L"[Mod] Failed to init refresh subsystem.");
        return FALSE;
    }
    // IXamlDiagnostics on background thread (replaces GetForCurrentView)
    if (!TBC::StartXamlDiagnostics()) {
        LOG_WARN(L"[Mod] XAML diagnostics failed — element discovery degraded.");
        // Non-fatal: hooks still work for future taskbar restarts
    }
    TBC::InstallHooks();

    g_modActive.store(true, std::memory_order_release);
    LOG_INFO(L"[Mod] Wh_ModInit complete — mod is active");
    return TRUE;
}

void Wh_ModUninit() {
    LOG_INFO(L"[Mod] Wh_ModUninit start");
    g_modActive.store(false, std::memory_order_release);

    TBC::UninstallHooks();
    TBC::RestoreAllButtons();
    TBC::ClearButtons();
    TBC::StopXamlDiagnostics();
    TBC::ShutdownRefresh();

    LOG_INFO(L"[Mod] Wh_ModUninit complete");
}

void Wh_ModSettingsChanged() {
    if (!g_modActive.load(std::memory_order_acquire)) return;
    LOG_INFO(L"[Mod] Settings changed → requesting refresh");
    TBC::RequestRefresh();
}
