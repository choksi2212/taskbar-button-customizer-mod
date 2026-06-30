# Testing Guide

## Manual Test Cases

All test cases should pass before any release.

---

### 1. OS Version Guard

| Test | Expected |
|---|---|
| Load mod on Windows 10 | Mod refuses to load; Windhawk log shows warning |
| Load mod on Windows 11 22H2 (build 22621) | Loads and applies |
| Load mod on Windows 11 23H2 (build 22631) | Loads and applies |
| Load mod on Windows 11 24H2 (build 26100) | Loads and applies |

---

### 2. Settings — Range Validation

| Input | Expected |
|---|---|
| `buttonWidth = -1` | Validation failure logged; value clamped/skipped |
| `backgroundOpacity = 1.5` | Validation failure logged; clamped to 1.0 |
| `buttonMinWidth = 200, buttonMaxWidth = 100` | Cross-field failure logged |
| `iconScale = 0` | Validation failure; value skipped |

---

### 3. Button Geometry

| Setting | Change | Expected |
|---|---|---|
| buttonWidth | 56 → 72 | Buttons visibly widen |
| buttonHeight | 48 → 60 | Buttons visibly taller |
| buttonWidth | 56 → 0 | Falls back to Windows default |

---

### 4. Corner Radius

| Setting | Change | Expected |
|---|---|---|
| cornerRadius | 4 → 0 | Square buttons |
| cornerRadius | 4 → 24 | Pill-shaped buttons |
| cornerRadius | 4 → 100 | Fully rounded |

---

### 5. Icon

| Setting | Expected |
|---|---|
| iconSize → 16 | Icons visibly smaller |
| iconOffsetY → -4 | Icons shift upward |
| iconScale → 0.8 | Icons scaled down relative to button |

---

### 6. Running Indicator

| Setting | Expected |
|---|---|
| indicatorHeight → 2 | Thinner dot/bar under active apps |
| indicatorWidth → 24 | Wider indicator |
| indicatorCornerRadius → 0 | Square indicator |

---

### 7. Live Refresh

| Action | Expected |
|---|---|
| Change any setting and save | Taskbar updates within 1–2 seconds with NO Explorer restart |
| Launch a new app during mod load | New button receives the same styling |

---

### 8. Profiles

| Profile | Key expectations |
|---|---|
| Compact | Buttons 40×40, icon 20px |
| Large | Buttons 72×56, icon 32px |
| Minimal | Transparent background, tiny indicator |
| Rounded | CornerRadius 24px |
| Gaming | Larger buttons, hover scale 1.08 |

---

### 9. Multi-Monitor

| Setup | Expected |
|---|---|
| 2 monitors with extended taskbar | Both taskbars styled |
| Secondary taskbar only | No crash, secondary styled if discovered |

---

### 10. Mod Unload / Reload

| Action | Expected |
|---|---|
| Disable mod in Windhawk | Taskbar returns to Windows defaults |
| Re-enable mod | Settings re-applied, no crash |
| Explorer restart with mod enabled | Mod auto-reloads and re-applies |

---

### 11. HiDPI

| Scale | Expected |
|---|---|
| 100% | Correct pixel values |
| 125% | Scaling applied by XAML; no visual artifacts |
| 150% | Same |
| 200% | Same |

---

### 12. Theme

| Scenario | Expected |
|---|---|
| Light theme | Settings apply; no color regression |
| Dark theme | Same |
| Switch theme while mod active | Mod survives; styling preserved |

---

### 13. Sleep / Wake

| Action | Expected |
|---|---|
| Sleep PC → wake | Taskbar re-applies styling on next SetWindowPos callback |

---

### 14. Virtual Desktops

| Action | Expected |
|---|---|
| Switch virtual desktops | No crash; styling consistent |
| Create new virtual desktop | No crash |
