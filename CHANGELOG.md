# Changelog

All notable changes to this project will be documented in this file.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [1.0.0] — 2026-06-30

### Added

- **Button geometry**: Width, Height, MinWidth, MaxWidth controls.
- **Appearance**: CornerRadius, PaddingH/V, MarginH/V, BackgroundOpacity, BorderThickness.
- **Icon**: Size, OffsetX/Y, Opacity, Scale controls.
- **Running indicator**: Height, Width, CornerRadius.
- **Hover state**: Scale, Opacity, Animation Duration.
- **Pressed state**: Scale, Opacity, Animation Duration.
- **Spacing**: Configurable gap between taskbar buttons.
- **8 built-in profiles**: Custom, Compact, Large, Minimal, Rounded, Gaming, Developer, Tablet.
- **Live refresh**: Settings apply instantly without restarting Explorer.
- **OS version guard**: Refuses to load on unsupported builds (< 22621).
- **Graceful error handling**: Per-property failures are logged and skipped without crashing Explorer.
- **Full settings validation**: Range checks, cross-field constraints, descriptive error messages.
- **Modular architecture**: 6 internal components (ExplorerHooks, TaskbarDiscovery, PropertyEngine, SettingsManager, LiveRefreshManager, Logger).
- MIT license.

---

## Upcoming — v1.1.0

- Per-state (hover/pressed/selected) visual state animations via XAML Storyboard.
- Expanded color controls for the running indicator.

## Upcoming — v1.2.0

- Profile import/export (JSON/YAML).
- Animation presets (macOS Dock, Windows 10, Floating, Material, Fluent).

## Upcoming — v2.0.0

- Full visual designer with live preview.
- Theme editor and community theme support.
