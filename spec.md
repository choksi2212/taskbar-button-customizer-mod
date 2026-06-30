Absolutely. If you're planning to make this open source, I'd recommend treating it as a proper engineering project rather than "just another Windhawk mod." Below is a detailed design document that can serve as the project specification.

---

# Windows 11 Taskbar Button Customizer (Windhawk Mod)

**Version:** 1.0 (Project Specification)

**Author:** Manas Choksi (Suggested)

**License:** MIT

**Platform:** Windows 11 (23H2, 24H2, 25H2)

**Framework:** Windhawk C++ Mod

---

# 1. Project Overview

## Purpose

Develop a Windhawk mod that allows users to customize the Windows 11 taskbar button container (the rounded transparent box behind each taskbar icon).

The goal is to expose customization options that Microsoft does not provide, while keeping Explorer stable and maintaining compatibility across Windows 11 versions.

The mod should be easy to use, open source, and suitable for submission to the official Windhawk gallery.

---

# 2. Problem Statement

Windows 11 users can customize:

* icon size
* taskbar alignment
* combine labels

However, Microsoft does not allow changing:

* taskbar button width
* taskbar button height
* corner radius
* hover animation
* padding
* background opacity
* indicator size
* spacing
* margins

Users currently rely on:

* Windows 11 Taskbar Styler
* ExplorerPatcher
* StartAllBack

These solutions either require manual editing of XAML selectors or don't expose the required settings.

This project solves that problem.

---

# 3. Objectives

Primary objectives:

✔ Resize taskbar button

✔ Resize hover container

✔ Resize pressed container

✔ Resize selected container

✔ Resize running indicator

✔ Control padding

✔ Control margins

✔ Control corner radius

✔ Control icon positioning

✔ Keep Explorer stable

---

# 4. Target Users

* Windows customization enthusiasts
* Windhawk users
* UI designers
* Developers
* Power users
* Theme creators

---

# 5. Compatibility

Supported:

* Windows 11 23H2
* Windows 11 24H2
* Windows 11 25H2

Architecture:

* x64

Not supported:

* Windows 10
* Windows Server
* ARM (initial release)

---

# 6. Technology Stack

## Primary

Language:

```
C++17
```

Framework:

```
Windhawk SDK
```

Compiler:

```
MSVC
```

IDE:

```
Visual Studio 2022
```

---

# Libraries

Windhawk SDK

Windows SDK

WinRT

COM

XAML Diagnostics

---

# Debugging Tools

Visual Studio Debugger

WinDbg

Spy++

UWPSpy (optional)

Process Explorer

---

# Repository Structure

```
TaskbarButtonCustomizer/

│
├── src/
│     mod.cpp
│     hooks.cpp
│     xaml.cpp
│     settings.cpp
│
├── include/
│
├── docs/
│
├── screenshots/
│
├── README.md
│
├── LICENSE
│
└── CHANGELOG.md
```

---

# 7. High Level Architecture

```
Explorer.exe

↓

Taskbar

↓

Taskbar XAML Tree

↓

Windhawk Injection

↓

Hook Taskbar Controls

↓

Read User Settings

↓

Modify Properties

↓

Render Updated Button
```

---

# 8. Internal Components

## Component 1

Explorer Hook

Responsibilities

* inject into explorer
* initialize mod
* detect Windows version
* locate taskbar

---

## Component 2

Taskbar Discovery

Responsibilities

Locate

TaskbarView

TaskbarButton

TaskListButton

ItemsRepeater

Border

Grid

---

## Component 3

Property Engine

Responsible for changing

Width

Height

Padding

Margin

CornerRadius

Opacity

Scale

Transform

---

## Component 4

Settings Engine

Reads Windhawk settings

Example

```yaml
ButtonWidth: 60

ButtonHeight: 48

CornerRadius: 12

PaddingX: 8

PaddingY: 4

IndicatorHeight: 3
```

---

## Component 5

Animation Engine

Controls

Hover

Pressed

Selected

Animations

---

## Component 6

Live Refresh

When user changes a setting

↓

Immediately update taskbar

↓

No Explorer restart

---

# 9. Features

## Button

Width

Height

Minimum width

Maximum width

Padding

Margin

Corner radius

Opacity

Border

Border thickness

Shadow

Blur

---

## Icon

Icon Size

Offset X

Offset Y

Opacity

Scale

Rotation (optional)

---

## Running Indicator

Height

Width

Corner Radius

Color

Position

Animation

---

## Hover

Hover opacity

Hover animation

Hover duration

Hover scale

Hover color

---

## Pressed

Pressed animation

Pressed scale

Pressed opacity

Pressed duration

---

## Selected

Background

Indicator

Scale

Border

---

## Spacing

Between buttons

Between groups

Between pinned icons

Between running icons

---

# 10. Advanced Features

Animation presets

```
Windows Default

macOS Dock

Windows 10

Minimal

Rounded

Floating

Material

Fluent
```

---

Profiles

```
Gaming

Compact

Large

Tablet

Minimal

Developer
```

---

Import

Export

Profiles

```
JSON

YAML
```

---

# 11. Settings UI

Windhawk Settings

```
Appearance

Button Width

[ 56 ]

Button Height

[ 48 ]

Corner Radius

[ 10 ]

Opacity

[ 85% ]
```

---

Hover

```
Enable Hover Animation

☑

Duration

[120]

Scale

[1.08]
```

---

Icon

```
Size

[24]

Offset X

[0]

Offset Y

[-2]
```

---

Indicator

```
Height

[3]

Width

[24]

Radius

[4]
```

---

# 12. Configuration File

```
ButtonWidth

ButtonHeight

PaddingHorizontal

PaddingVertical

MarginHorizontal

MarginVertical

CornerRadius

Opacity

HoverScale

HoverDuration

IndicatorHeight

IndicatorWidth

IconSize

IconOffsetX

IconOffsetY
```

---

# 13. Internal Hook Flow

```
Explorer Starts

↓

Windhawk Loads

↓

Find Taskbar

↓

Locate XAML Controls

↓

Read Settings

↓

Modify Dependency Properties

↓

Taskbar Redraw
```

---

# 14. Error Handling

If Windows build unsupported

↓

Disable mod

↓

Show warning

If taskbar class not found

↓

Retry

↓

Disable gracefully

If Explorer crashes

↓

Unload

↓

Restore defaults

---

# 15. Performance Goals

Explorer startup delay

<20 ms

Memory

<10 MB

CPU

≈0%

---

# 16. Testing Matrix

Test:

* Single monitor
* Dual monitor
* Triple monitor
* Light theme
* Dark theme
* Small taskbar
* Large taskbar
* Center icons
* Left aligned
* Explorer restart
* Sleep/Wake
* Virtual desktops
* High DPI (100%, 125%, 150%, 200%)

---

# 17. Documentation

README should include:

* Features
* Installation
* Screenshots
* Compatibility
* Known issues
* FAQ
* Building from source
* Contributing
* Changelog

---

# 18. Roadmap

### Version 1.0

* Button size
* Corner radius
* Padding
* Margins
* Indicator size

### Version 1.1

* Hover effects
* Pressed effects
* Selected effects

### Version 1.2

* Live animations
* Profiles
* Import/export

### Version 2.0

* Full taskbar button designer
* Theme editor
* Animation editor
* Visual preview
* Community theme support

---

# 19. Development Strategy

Rather than hardcoding offsets or relying on fragile memory patches, the mod should:

* Detect the Windows build at runtime.
* Hook into Explorer using the official Windhawk APIs.
* Locate the relevant XAML elements dynamically where possible.
* Apply property changes through supported XAML dependency properties instead of patching Explorer binaries.
* Fail gracefully if a future Windows update changes the taskbar internals.

This approach will make the mod easier to maintain and more resilient across Windows updates.

---

## Suggested Milestones

1. **Research Phase**

   * Study the existing Windhawk SDK and official sample mods.
   * Review the Windows 11 Taskbar Styler implementation to understand how it locates and modifies taskbar XAML.

2. **Prototype**

   * Build a minimal mod that can find the taskbar button element and log its discovery.
   * Verify compatibility on Windows 25H2.

3. **Core Customization**

   * Implement width, height, padding, margin, and corner radius controls.

4. **Settings Integration**

   * Expose options through Windhawk's settings UI with validation and sensible defaults.

5. **Advanced Features**

   * Add hover, pressed, and animation customization.
   * Implement profile import/export.

6. **Testing & Release**

   * Test across supported Windows versions and display configurations.
   * Publish the source code, documentation, and screenshots.
   * Submit to the Windhawk gallery.

This plan should give you a professional-grade project with a clear roadmap from proof of concept to a polished, community-ready Windhawk mod.
