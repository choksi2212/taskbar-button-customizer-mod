// =============================================================================
// hooks.h — Taskbar Button Customizer
// Declares ExplorerHooks: installs and removes Windhawk function hooks that
// let the mod intercept the taskbar's creation and update lifecycle.
//
// Hook strategy
// ─────────────
// Rather than hooking arbitrary internal Explorer functions (which changes
// between Windows builds), we hook:
//   1. CreateWindowExW — to detect when Explorer creates the taskbar window.
//   2. SetWindowPos    — to detect taskbar layout changes (e.g. after DPI
//                        change, taskbar resize, or icon addition).
//
// Both are stable Win32 APIs that will not change across Windows builds.
// After detecting the relevant windows, the Windhawk XAML layer locates
// the XAML elements and applies settings.
// =============================================================================
#pragma once

#include <windows.h>
#include <functional>

namespace TaskbarCustomizer {

// Callback types
using TaskbarCreatedCallback = std::function<void(HWND taskbarHwnd)>;
using TaskbarUpdatedCallback = std::function<void(HWND hwnd)>;

// ---------------------------------------------------------------------------
// ExplorerHooks — manages the lifecycle of all Windhawk function hooks.
// ---------------------------------------------------------------------------
class ExplorerHooks {
public:
    ExplorerHooks() = delete;

    /// Install all hooks.  Must be called from Wh_ModInit on the main thread.
    /// Returns true on success; false if any critical hook fails.
    static bool Install(
        TaskbarCreatedCallback onCreated,
        TaskbarUpdatedCallback onUpdated) noexcept;

    /// Remove all hooks.  Must be called from Wh_ModUninit.
    static void Uninstall() noexcept;

    /// Returns true if hooks are currently installed.
    static bool IsInstalled() noexcept { return s_installed; }

private:
    // ── Stored callbacks ────────────────────────────────────────────────────
    static TaskbarCreatedCallback s_onTaskbarCreated;
    static TaskbarUpdatedCallback s_onTaskbarUpdated;
    static bool                  s_installed;

    // ── Hook trampoline functions ────────────────────────────────────────────
    static HWND WINAPI HookedCreateWindowExW(
        DWORD     dwExStyle,
        LPCWSTR   lpClassName,
        LPCWSTR   lpWindowName,
        DWORD     dwStyle,
        int       X, int Y, int nWidth, int nHeight,
        HWND      hWndParent,
        HMENU     hMenu,
        HINSTANCE hInstance,
        LPVOID    lpParam);

    static BOOL WINAPI HookedSetWindowPos(
        HWND hWnd,
        HWND hWndInsertAfter,
        int  X, int Y, int cx, int cy,
        UINT uFlags);

    // ── Original function pointers ───────────────────────────────────────────
    static decltype(&::CreateWindowExW) s_origCreateWindowExW;
    static decltype(&::SetWindowPos)    s_origSetWindowPos;

    // ── Taskbar class name filter ─────────────────────────────────────────────
    static bool IsTaskbarWindowClass(LPCWSTR lpClassName) noexcept;
};

} // namespace TaskbarCustomizer
