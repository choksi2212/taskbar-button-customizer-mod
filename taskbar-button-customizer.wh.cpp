// =============================================================================
// mod.cpp — Taskbar Button Customizer (Windhawk Mod)
// Main entry point: Windhawk metadata, settings schema, and lifecycle hooks.
//
// Windhawk calls:
//   Wh_ModInit()           — on load; install hooks, initial apply
//   Wh_ModUninit()         — on unload; remove hooks, restore defaults
//   Wh_ModSettingsChanged()— when the user saves a settings change; re-apply
//   Wh_ModBeforeUninit()   — (optional) before uninit; no action needed here
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

## Settings

All options are accessible in the Windhawk settings panel.

## Compatibility

- Windows 11 23H2 (build 22631+)
- Windows 11 24H2 (build 26100+)
- Windows 11 25H2 (anticipated)
- x64 only

## Open Source

Source code: https://github.com/choksi2212/taskbar-button-customizer-mod
License: MIT
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- profile: Custom
  $name: Profile
  $description: >-
    Select a built-in layout profile. Choosing any profile other than Custom
    will override the individual settings below with preset values.
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
  $description: >-
    Width of each taskbar button in pixels. Set to 0 to use the Windows default.

- buttonHeight: "48"
  $name: Button Height (px)
  $description: >-
    Height of each taskbar button in pixels. Set to 0 to use the Windows default.

- buttonMinWidth: "32"
  $name: Button Minimum Width (px)

- buttonMaxWidth: "160"
  $name: Button Maximum Width (px)

- cornerRadius: "4"
  $name: Corner Radius (px)
  $description: Applied to the button's background container (BackgroundElement).

- paddingHorizontal: "8"
  $name: Padding — Horizontal (px)

- paddingVertical: "4"
  $name: Padding — Vertical (px)

- marginHorizontal: "1"
  $name: Margin — Horizontal (px)

- marginVertical: "0"
  $name: Margin — Vertical (px)

- backgroundOpacity: "1.0"
  $name: Background Opacity
  $description: 0.0 = fully transparent, 1.0 = fully opaque.

- borderThickness: "0"
  $name: Border Thickness (px)

- iconSize: "24"
  $name: Icon Size (px)

- iconOffsetX: "0"
  $name: Icon Horizontal Offset (px)

- iconOffsetY: "0"
  $name: Icon Vertical Offset (px)

- iconOpacity: "1.0"
  $name: Icon Opacity
  $description: 0.0 = invisible, 1.0 = fully visible.

- iconScale: "1.0"
  $name: Icon Scale
  $description: 1.0 = normal. Values > 1 enlarge; < 1 shrink.

- indicatorHeight: "3"
  $name: Running Indicator Height (px)

- indicatorWidth: "16"
  $name: Running Indicator Width (px)

- indicatorCornerRadius: "2"
  $name: Running Indicator Corner Radius (px)

- hoverAnimationEnabled: true
  $name: Enable Hover Animation

- hoverScale: "1.0"
  $name: Hover Scale
  $description: >-
    Scale factor applied to the button on hover. 1.0 = no scale change.
    Values > 1 make the button grow slightly on hover.

- hoverOpacity: "1.0"
  $name: Hover Opacity

- hoverDurationMs: "150"
  $name: Hover Animation Duration (ms)

- pressedScale: "0.95"
  $name: Pressed Scale

- pressedOpacity: "0.9"
  $name: Pressed Opacity

- pressedDurationMs: "80"
  $name: Pressed Animation Duration (ms)

- spacingBetweenButtons: "2"
  $name: Spacing Between Buttons (px)
*/
// ==/WindhawkModSettings==

// ─── Implementation headers ───────────────────────────────────────────────────
#include "src/logger.h"
#include "src/version_compat.h"
#include "src/settings.h"
#include "src/xaml_utils.h"
#include "src/property_engine.h"
#include "src/taskbar_discovery.h"
#include "src/hooks.h"
#include "src/live_refresh.h"

#include <windhawk_api.h>
#include <windows.h>
#include <atomic>

// ─── Module-level state ───────────────────────────────────────────────────────

namespace {

/// True once Wh_ModInit has succeeded and the mod is fully active.
std::atomic<bool> g_modActive { false };

/// Current settings snapshot (updated atomically on settings change).
/// Access under g_settingsMutex for writes; reads in Apply are done after
/// the mutex in LiveRefreshManager::ApplyNow which is always on the UI thread.
TaskbarCustomizer::ModSettings g_currentSettings;

// ─── Internal callbacks (called from hooks) ───────────────────────────────────

void OnTaskbarCreated(HWND /*taskbarHwnd*/) {
    LOG_INFO(L"[Mod] Taskbar created; scheduling initial apply.");
    TaskbarCustomizer::LiveRefreshManager::RequestRefresh();
}

void OnTaskbarUpdated(HWND /*hwnd*/) {
    LOG_DEBUG(L"[Mod] Taskbar updated; scheduling re-apply.");
    TaskbarCustomizer::LiveRefreshManager::RequestRefresh();
}

} // anonymous namespace

// ─── Windhawk entry points ────────────────────────────────────────────────────

/// Called by Windhawk when the mod is loaded into Explorer.exe.
BOOL Wh_ModInit() {
    LOG_INFO(L"[Mod] Wh_ModInit start.");

    // ── Step 1: OS version check ─────────────────────────────────────────────
    if (!TaskbarCustomizer::IsOsVersionSupported()) {
        LOG_ERROR(L"[Mod] Unsupported OS version. Mod will not load.");
        return FALSE; // Signal Windhawk to not proceed with this mod.
    }

    // ── Step 2: Initialize the live-refresh subsystem ────────────────────────
    if (!TaskbarCustomizer::LiveRefreshManager::Initialize()) {
        LOG_ERROR(L"[Mod] Failed to initialize LiveRefreshManager.");
        return FALSE;
    }

    // ── Step 3: Register the discovery callback for new buttons ──────────────
    TaskbarCustomizer::TaskbarDiscovery::SetButtonFoundCallback(
        [](TaskbarCustomizer::winrt::Windows::UI::Xaml::DependencyObject btn) {
            // A new button appeared — load current settings and apply
            TaskbarCustomizer::ValidationResult vr;
            auto s = TaskbarCustomizer::SettingsManager::LoadAndValidate(vr);
            if (!s.profile.empty() && s.profile != L"Custom") {
                TaskbarCustomizer::SettingsManager::ApplyProfile(s.profile, s);
            }
            TaskbarCustomizer::PropertyEngine::ApplyToButtonSubtree(btn, s);
        });

    // ── Step 4: Install Win32 API hooks ──────────────────────────────────────
    if (!TaskbarCustomizer::ExplorerHooks::Install(
            OnTaskbarCreated, OnTaskbarUpdated))
    {
        LOG_WARN(L"[Mod] Some hooks failed; mod may not fully function.");
        // Non-fatal — we still try an immediate apply below.
    }

    // ── Step 5: Immediate apply (taskbar may already be running) ─────────────
    TaskbarCustomizer::LiveRefreshManager::RequestRefresh();

    g_modActive.store(true, std::memory_order_release);
    LOG_INFO(L"[Mod] Wh_ModInit complete — mod is active.");
    return TRUE;
}

/// Called by Windhawk before the mod is unloaded.
void Wh_ModUninit() {
    LOG_INFO(L"[Mod] Wh_ModUninit start.");

    g_modActive.store(false, std::memory_order_release);

    // ── Remove hooks ─────────────────────────────────────────────────────────
    TaskbarCustomizer::ExplorerHooks::Uninstall();

    // ── Restore defaults on all discovered buttons ────────────────────────────
    auto elements = TaskbarCustomizer::TaskbarDiscovery::DiscoverAll();
    if (elements.has_value()) {
        for (auto& btn : elements->taskListButtons) {
            TaskbarCustomizer::PropertyEngine::RestoreButtonSubtree(btn);
        }
        LOG_INFO(L"[Mod] Restored %zu buttons to Windows defaults.",
                 elements->taskListButtons.size());
    }

    // ── Shut down live refresh ────────────────────────────────────────────────
    TaskbarCustomizer::LiveRefreshManager::Shutdown();

    LOG_INFO(L"[Mod] Wh_ModUninit complete.");
}

/// Called by Windhawk when the user saves new settings in the UI panel.
void Wh_ModSettingsChanged() {
    if (!g_modActive.load(std::memory_order_acquire)) return;

    LOG_INFO(L"[Mod] Settings changed; requesting refresh.");
    TaskbarCustomizer::LiveRefreshManager::RequestRefresh();
}
