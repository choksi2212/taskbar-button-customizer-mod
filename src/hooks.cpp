// =============================================================================
// hooks.cpp — Taskbar Button Customizer
// Implements ExplorerHooks: installs hooks on CreateWindowExW and SetWindowPos.
//
// Thread safety
// ─────────────
// Windhawk guarantees that Wh_ModInit/Uninit are called on the main thread.
// The hook trampolines may be called on any thread; we use atomic booleans
// and the callbacks are safe to call from any thread context.
// =============================================================================
#include "hooks.h"
#include "logger.h"

#include <windhawk_api.h>
#include <windhawk_utils.h>
#include <atomic>
#include <string_view>

namespace TaskbarCustomizer {

// ── Static member definitions ─────────────────────────────────────────────────

TaskbarCreatedCallback ExplorerHooks::s_onTaskbarCreated;
TaskbarUpdatedCallback ExplorerHooks::s_onTaskbarUpdated;
bool                   ExplorerHooks::s_installed = false;

decltype(&::CreateWindowExW) ExplorerHooks::s_origCreateWindowExW = nullptr;
decltype(&::SetWindowPos)    ExplorerHooks::s_origSetWindowPos    = nullptr;

// ── Known taskbar window class names ─────────────────────────────────────────

static constexpr std::wstring_view kTaskbarClasses[] = {
    L"Shell_TrayWnd",
    L"Shell_SecondaryTrayWnd",
    L"Windows.UI.Composition.DesktopWindowContentBridge",
    L"Microsoft.UI.Content.DesktopChildSiteBridge",
};

bool ExplorerHooks::IsTaskbarWindowClass(LPCWSTR lpClassName) noexcept {
    if (!lpClassName) return false;
    // lpClassName may be an atom (MAKEINTATOM); skip those
    if (reinterpret_cast<uintptr_t>(lpClassName) <= 0xFFFF) return false;

    std::wstring_view cls{ lpClassName };
    for (auto& known : kTaskbarClasses) {
        if (cls == known) return true;
    }
    return false;
}

// ── Hook trampolines ──────────────────────────────────────────────────────────

HWND WINAPI ExplorerHooks::HookedCreateWindowExW(
    DWORD     dwExStyle,
    LPCWSTR   lpClassName,
    LPCWSTR   lpWindowName,
    DWORD     dwStyle,
    int       X, int Y, int nWidth, int nHeight,
    HWND      hWndParent,
    HMENU     hMenu,
    HINSTANCE hInstance,
    LPVOID    lpParam)
{
    // Delegate to the original first so the window is fully created.
    HWND hwnd = s_origCreateWindowExW(
        dwExStyle, lpClassName, lpWindowName,
        dwStyle, X, Y, nWidth, nHeight,
        hWndParent, hMenu, hInstance, lpParam);

    if (hwnd && IsTaskbarWindowClass(lpClassName)) {
        // Only fire for the MAIN taskbar (Shell_TrayWnd)
        wchar_t buf[256] = {};
        ::GetClassNameW(hwnd, buf, _countof(buf));
        if (::wcscmp(buf, L"Shell_TrayWnd") == 0) {
            LOG_INFO(L"[Hooks] Shell_TrayWnd created: 0x%p", hwnd);
            if (s_onTaskbarCreated) {
                s_onTaskbarCreated(hwnd);
            }
        } else {
            // Secondary taskbar or XAML bridge — notify "updated"
            LOG_DEBUG(L"[Hooks] Taskbar-class window created (%ls): 0x%p",
                      buf, hwnd);
            if (s_onTaskbarUpdated) {
                s_onTaskbarUpdated(hwnd);
            }
        }
    }

    return hwnd;
}

BOOL WINAPI ExplorerHooks::HookedSetWindowPos(
    HWND hWnd,
    HWND hWndInsertAfter,
    int  X, int Y, int cx, int cy,
    UINT uFlags)
{
    BOOL result = s_origSetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);

    if (result && hWnd) {
        // Check if this is the taskbar being repositioned / resized
        wchar_t cls[256] = {};
        ::GetClassNameW(hWnd, cls, _countof(cls));
        if (::wcscmp(cls, L"Shell_TrayWnd") == 0) {
            LOG_DEBUG(L"[Hooks] SetWindowPos on Shell_TrayWnd: cx=%d cy=%d", cx, cy);
            if (s_onTaskbarUpdated) {
                s_onTaskbarUpdated(hWnd);
            }
        }
    }

    return result;
}

// ── Install / Uninstall ───────────────────────────────────────────────────────

bool ExplorerHooks::Install(
    TaskbarCreatedCallback onCreated,
    TaskbarUpdatedCallback onUpdated) noexcept
{
    if (s_installed) {
        LOG_WARN(L"[Hooks] Install called while already installed; ignoring.");
        return true;
    }

    s_onTaskbarCreated = std::move(onCreated);
    s_onTaskbarUpdated = std::move(onUpdated);

    bool ok = true;

    // Hook CreateWindowExW (user32.dll)
    void* origCreate = nullptr;
    if (!Wh_SetFunctionHook(
            reinterpret_cast<void*>(&::CreateWindowExW),
            reinterpret_cast<void*>(&HookedCreateWindowExW),
            &origCreate))
    {
        LOG_ERROR(L"[Hooks] Failed to hook CreateWindowExW.");
        ok = false;
    } else {
        s_origCreateWindowExW = reinterpret_cast<decltype(s_origCreateWindowExW)>(
            origCreate);
        LOG_INFO(L"[Hooks] CreateWindowExW hooked successfully.");
    }

    // Hook SetWindowPos (user32.dll)
    void* origSetWP = nullptr;
    if (!Wh_SetFunctionHook(
            reinterpret_cast<void*>(&::SetWindowPos),
            reinterpret_cast<void*>(&HookedSetWindowPos),
            &origSetWP))
    {
        LOG_ERROR(L"[Hooks] Failed to hook SetWindowPos.");
        ok = false; // non-fatal; live-resize won't trigger but initial load still works
    } else {
        s_origSetWindowPos = reinterpret_cast<decltype(s_origSetWindowPos)>(
            origSetWP);
        LOG_INFO(L"[Hooks] SetWindowPos hooked successfully.");
    }

    // Apply all registered hooks
    Wh_ApplyHookOperations();

    s_installed = ok;
    return ok;
}

void ExplorerHooks::Uninstall() noexcept {
    if (!s_installed) return;

    // Windhawk automatically removes all hooks registered with Wh_SetFunctionHook
    // when Wh_ModUninit returns; we just clear our state here.
    s_onTaskbarCreated = nullptr;
    s_onTaskbarUpdated = nullptr;
    s_origCreateWindowExW = nullptr;
    s_origSetWindowPos    = nullptr;
    s_installed = false;

    LOG_INFO(L"[Hooks] Hooks uninstalled.");
}

} // namespace TaskbarCustomizer
