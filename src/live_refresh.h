// =============================================================================
// live_refresh.h — Taskbar Button Customizer
// Declares LiveRefreshManager: applies (or re-applies) settings without
// restarting Explorer.exe.
//
// Refresh triggers:
//   1. Wh_ModSettingsChanged is called by Windhawk when the user saves settings.
//   2. A new taskbar button appears (new app launched) — caught via the
//      ButtonFoundCallback registered in TaskbarDiscovery.
//   3. The taskbar HWND is repositioned (SetWindowPos hook callback).
//
// All refreshes are marshalled to the UI thread via a PostMessage approach
// using a hidden message window so that XAML writes happen on the correct
// thread.
// =============================================================================
#pragma once

#include <windows.h>
#include "settings.h"

namespace TaskbarCustomizer {

// ---------------------------------------------------------------------------
// LiveRefreshManager
// ---------------------------------------------------------------------------
class LiveRefreshManager {
public:
    LiveRefreshManager() = delete;

    /// Initialize the refresh machinery (creates the message window).
    /// Call from Wh_ModInit.
    static bool Initialize() noexcept;

    /// Tear down the refresh machinery.
    /// Call from Wh_ModUninit.
    static void Shutdown() noexcept;

    /// Schedule an immediate full refresh with the current settings.
    /// Thread-safe; can be called from any thread.
    static void RequestRefresh() noexcept;

    /// Immediately apply settings (must be called on the XAML UI thread).
    /// Called internally by the message window procedure.
    static void ApplyNow() noexcept;

private:
    // ── Message window ───────────────────────────────────────────────────────
    static HWND     s_messageWindow;
    static ATOM     s_wndClass;

    static constexpr UINT WM_TBC_REFRESH = WM_USER + 1;

    static LRESULT CALLBACK MessageWindowProc(
        HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static bool CreateMessageWindow() noexcept;
    static void DestroyMessageWindow() noexcept;
};

} // namespace TaskbarCustomizer
