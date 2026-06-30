# Architecture

## Overview

```
taskbar-button-customizer.wh.cpp   ← Windhawk entry point + settings schema
src/
  logger.h              ← Zero-overhead logging with severity levels
  version_compat.h      ← OS version detection (RtlGetVersion)
  settings.h/.cpp       ← ModSettings struct + SettingsManager
  xaml_utils.h          ← Visual tree traversal + DP helpers
  property_engine.h/.cpp← XAML property writes (the "what to change")
  taskbar_discovery.h/.cpp  ← HWND→XAML chain (the "where to change it")
  hooks.h/.cpp          ← Win32 API hooks (CreateWindowExW, SetWindowPos)
  live_refresh.h/.cpp   ← Thread-safe refresh via message-only window
```

## Data Flow

```
User saves settings in Windhawk UI
  ↓
Wh_ModSettingsChanged()
  ↓
LiveRefreshManager::RequestRefresh()   [PostMessage → UI thread]
  ↓
LiveRefreshManager::ApplyNow()         [UI thread]
  ↓
SettingsManager::LoadAndValidate()
  ↓
SettingsManager::ApplyProfile()        [if non-Custom profile]
  ↓
TaskbarDiscovery::DiscoverAll()
  ├─ FindTaskbarHwnd()          → Shell_TrayWnd
  ├─ FindXamlIslandHwnd()       → XAML Island child
  ├─ GetXamlRootFromHwnd()      → DesktopWindowXamlSource.Content
  └─ CollectTaskListButtons()   → [TaskListButton, ...]
  ↓
For each button:
  PropertyEngine::ApplyToButtonSubtree()
    ├─ ApplyButtonGeometry()    Width / Height / MinWidth / MaxWidth
    ├─ ApplyButtonAppearance()  CornerRadius / Padding / Margin / BorderThickness
    ├─ ApplyBackgroundOpacity() Opacity on BackgroundElement
    ├─ ApplyIconProperties()    Size / Offset / Opacity / Scale / Transform
    ├─ ApplyIndicatorProperties() Height / Width / RadiusX / RadiusY
    └─ ApplySpacing()           Margin on outer button element
```

## Threading Model

| Thread | Responsibility |
|---|---|
| Explorer main (UI) | Wh_ModInit, message window, XAML API calls |
| Any | RequestRefresh() — thread-safe via PostMessage |
| Hook callsite | HookedCreateWindowExW, HookedSetWindowPos → calls RequestRefresh() |

## Error Recovery

- Every `Apply*` method is `noexcept`.
- WinRT exceptions are caught and logged; the remainder of the apply continues.
- If the entire taskbar tree is unavailable, `ApplyNow` returns without crashing
  and the next hook-triggered refresh will retry.
- On mod unload, `RestoreButtonSubtree` clears all mod-owned properties,
  returning the taskbar to Windows defaults.
