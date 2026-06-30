# Windows 11 Taskbar Button Customizer — Windhawk Mod

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform: Windows 11](https://img.shields.io/badge/Platform-Windows%2011-blue)](https://www.microsoft.com/windows/windows-11)
[![Windhawk](https://img.shields.io/badge/Framework-Windhawk-purple)](https://windhawk.net)

> **Unlock every visual dimension of the Windows 11 taskbar button** that Microsoft keeps locked — button size, corner radius, padding, margins, icon positioning, running indicator, hover/press animations, and spacing — all from a clean settings panel with **live refresh** and no Explorer restart required.

---

## ✨ Features (v1.0)

| Category | Properties |
|---|---|
| **Button** | Width, Height, Min/Max Width |
| **Appearance** | Corner Radius, Padding (H/V), Margin (H/V), Background Opacity, Border Thickness |
| **Icon** | Size, Offset X/Y, Opacity, Scale |
| **Running Indicator** | Height, Width, Corner Radius |
| **Hover** | Scale, Opacity, Animation Duration |
| **Pressed** | Scale, Opacity, Animation Duration |
| **Spacing** | Between buttons |
| **Profiles** | Compact, Large, Minimal, Rounded, Gaming, Developer, Tablet |

---

## 📋 Requirements

| Item | Requirement |
|---|---|
| OS | Windows 11 23H2 (22631+), 24H2, or 25H2 |
| Architecture | x64 |
| Framework | [Windhawk](https://windhawk.net) v1.4+ |

Not supported on Windows 10, Windows Server, or ARM.

---

## 🚀 Installation

### Via Windhawk (recommended)

1. Install [Windhawk](https://windhawk.net).
2. Search for **"Taskbar Button Customizer"** in the mod browser.
3. Click **Install**.
4. Adjust settings in the **Settings** tab.

### Manual (from source)

1. Clone this repository.
2. Copy `taskbar-button-customizer.wh.cpp` (and the `src/` folder) to Windhawk's mod directory.
3. Build via **Compile mod** in the Windhawk app.

---

## 🎛️ Settings Reference

### Profile

| Profile | Description |
|---|---|
| Custom | Use all individual values below |
| Compact | Smaller buttons (40×40, icon 20px) |
| Large | Bigger buttons (72×56, icon 32px) |
| Minimal | Tiny, borderless, transparent |
| Rounded | High corner radius (24px) |
| Gaming | Slightly larger with hover scale |
| Developer | Clean, minimal margins |
| Tablet | Touch-friendly extra-large |

### Button Geometry

| Setting | Default | Description |
|---|---|---|
| Button Width | 56 | 0 = Windows default |
| Button Height | 48 | 0 = Windows default |
| Button Min Width | 32 | |
| Button Max Width | 160 | |
| Corner Radius | 4 | Applies to BackgroundElement border |

### Padding & Margin

| Setting | Default |
|---|---|
| Padding Horizontal | 8 |
| Padding Vertical | 4 |
| Margin Horizontal | 1 |
| Margin Vertical | 0 |

### Background

| Setting | Default |
|---|---|
| Background Opacity | 1.0 (fully opaque) |
| Border Thickness | 0 |

### Icon

| Setting | Default |
|---|---|
| Icon Size | 24 |
| Icon Offset X | 0 |
| Icon Offset Y | 0 |
| Icon Opacity | 1.0 |
| Icon Scale | 1.0 |

### Running Indicator

| Setting | Default |
|---|---|
| Indicator Height | 3 |
| Indicator Width | 16 |
| Indicator Corner Radius | 2 |

### Hover

| Setting | Default |
|---|---|
| Hover Animation Enabled | true |
| Hover Scale | 1.0 |
| Hover Opacity | 1.0 |
| Hover Duration (ms) | 150 |

### Pressed

| Setting | Default |
|---|---|
| Pressed Scale | 0.95 |
| Pressed Opacity | 0.9 |
| Pressed Duration (ms) | 80 |

---

## 🏗️ Architecture

```
Explorer.exe
  └─ Windhawk injection
       └─ ExplorerHooks (CreateWindowExW, SetWindowPos)
            └─ LiveRefreshManager (message-only window → UI thread)
                 └─ TaskbarDiscovery (Shell_TrayWnd → XAML Island → VisualTree)
                      └─ PropertyEngine (XAML DependencyProperty writes)
```

The mod uses **no binary offsets** and **no memory patches**.  All property
changes are applied through the public WinRT XAML API, making the mod
resilient to Windows updates.

---

## 🗺️ Roadmap

| Version | Planned Features |
|---|---|
| **v1.0** ✅ | Button size, corner radius, padding, margins, indicator, icon, spacing, profiles |
| **v1.1** | Per-state hover/pressed/selected visual effects |
| **v1.2** | Live storyboard animations, profile import/export (JSON/YAML) |
| **v2.0** | Full visual designer, theme editor, community theme support |

---

## 🧪 Testing Matrix

Tested configurations:

- [x] Single monitor
- [x] Dual monitor
- [x] Light theme
- [x] Dark theme
- [x] Small taskbar
- [x] Large taskbar
- [x] Centered icons
- [x] Left-aligned icons
- [x] After Explorer restart
- [x] Sleep/Wake cycle
- [x] Virtual desktops
- [x] HiDPI 100%, 125%, 150%, 200%

---

## ⚙️ Performance Goals

| Metric | Target | Notes |
|---|---|---|
| Explorer startup delay | < 20 ms | Measured on mid-range hardware |
| Memory overhead | < 10 MB | |
| CPU overhead (idle) | ≈ 0% | No polling; event-driven |

---

## 🛡️ Error Handling

| Failure | Behaviour |
|---|---|
| Unsupported Windows build | Mod refuses to load; logs warning |
| Taskbar not yet ready | Retry on next trigger (no crash) |
| XAML property write fails | Logged; other properties still applied |
| Explorer crash | Windhawk auto-unloads; defaults restored |

---

## 🔧 Building from Source

### Prerequisites

- Visual Studio 2022 with **Desktop development with C++**
- Windows 11 SDK (10.0.22621 or later)
- [Windhawk](https://windhawk.net) installed

### Build

Open Windhawk → **Develop a mod** → point to `taskbar-button-customizer.wh.cpp`.  
The Windhawk editor handles compilation and injection into Explorer.

---

## 🤝 Contributing

1. Fork the repository.
2. Create a feature branch: `git checkout -b feat/my-feature`
3. Commit changes: `git commit -m "feat: description"`
4. Push and open a Pull Request.

Please follow the existing code style (C++17, noexcept boundaries, LOG_* macros).

---

## 📄 License

MIT — see [LICENSE](LICENSE).

---

## 🙏 Acknowledgements

- [Windhawk](https://windhawk.net) by m417z — the mod injection framework
- [TranslucentTB / ExplorerTAP](https://github.com/TranslucentTB/TranslucentTB) — XAML diagnostics patterns
- [Windows 11 Taskbar Styler](https://github.com/ramensoftware/windhawk-mods) — XAML element names reference
- [UWPSpy](https://ramensoftware.com/uwpspy) — visual tree inspection tool
