// =============================================================================
// live_refresh.cpp — Taskbar Button Customizer
// Implements LiveRefreshManager.
//
// When RequestRefresh() is called from any thread, it posts WM_TBC_REFRESH
// to the hidden message window.  The message window procedure runs on the
// thread that called CreateMessageWindow(), which is the Wh_ModInit thread
// (the Explorer main thread).  This ensures all XAML API calls happen on
// the UI thread.
// =============================================================================
#include "live_refresh.h"
#include "taskbar_discovery.h"
#include "property_engine.h"
#include "settings.h"
#include "logger.h"

#include <windhawk_api.h>
#include <atomic>

namespace TaskbarCustomizer {

// ── Static member definitions ─────────────────────────────────────────────────

HWND LiveRefreshManager::s_messageWindow = nullptr;
ATOM LiveRefreshManager::s_wndClass      = 0;

// ── MessageWindowProc ─────────────────────────────────────────────────────────

LRESULT CALLBACK LiveRefreshManager::MessageWindowProc(
    HWND hwnd, UINT msg, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if (msg == WM_TBC_REFRESH) {
        LOG_DEBUG(L"[LiveRefresh] WM_TBC_REFRESH received; applying settings.");
        ApplyNow();
        return 0;
    }
    return ::DefWindowProcW(hwnd, msg, 0, 0);
}

// ── CreateMessageWindow ───────────────────────────────────────────────────────

bool LiveRefreshManager::CreateMessageWindow() noexcept {
    if (s_messageWindow) return true; // already created

    // Register message-only window class
    WNDCLASSEXW wc  = {};
    wc.cbSize       = sizeof(wc);
    wc.lpfnWndProc  = MessageWindowProc;
    wc.hInstance    = ::GetModuleHandleW(nullptr);
    wc.lpszClassName= L"TBC_LiveRefreshMsg";

    s_wndClass = ::RegisterClassExW(&wc);
    if (!s_wndClass) {
        DWORD err = ::GetLastError();
        // ERROR_CLASS_ALREADY_EXISTS is fine (mod was hot-reloaded)
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            LOG_ERROR(L"[LiveRefresh] RegisterClassExW failed: %lu", err);
            return false;
        }
    }

    // Create message-only window (HWND_MESSAGE parent → no visible window)
    s_messageWindow = ::CreateWindowExW(
        0, L"TBC_LiveRefreshMsg", nullptr, 0,
        0, 0, 0, 0,
        HWND_MESSAGE, nullptr, ::GetModuleHandleW(nullptr), nullptr);

    if (!s_messageWindow) {
        LOG_ERROR(L"[LiveRefresh] CreateWindowExW failed: %lu", ::GetLastError());
        return false;
    }

    LOG_INFO(L"[LiveRefresh] Message window created: 0x%p", s_messageWindow);
    return true;
}

// ── DestroyMessageWindow ──────────────────────────────────────────────────────

void LiveRefreshManager::DestroyMessageWindow() noexcept {
    if (s_messageWindow) {
        ::DestroyWindow(s_messageWindow);
        s_messageWindow = nullptr;
        LOG_INFO(L"[LiveRefresh] Message window destroyed.");
    }
    if (s_wndClass) {
        ::UnregisterClassW(L"TBC_LiveRefreshMsg",
                           ::GetModuleHandleW(nullptr));
        s_wndClass = 0;
    }
}

// ── Initialize ────────────────────────────────────────────────────────────────

bool LiveRefreshManager::Initialize() noexcept {
    return CreateMessageWindow();
}

// ── Shutdown ──────────────────────────────────────────────────────────────────

void LiveRefreshManager::Shutdown() noexcept {
    DestroyMessageWindow();
}

// ── RequestRefresh ────────────────────────────────────────────────────────────

void LiveRefreshManager::RequestRefresh() noexcept {
    if (!s_messageWindow) {
        LOG_WARN(L"[LiveRefresh] RequestRefresh called before Initialize().");
        return;
    }
    ::PostMessageW(s_messageWindow, WM_TBC_REFRESH, 0, 0);
    LOG_DEBUG(L"[LiveRefresh] Refresh requested.");
}

// ── ApplyNow ──────────────────────────────────────────────────────────────────

void LiveRefreshManager::ApplyNow() noexcept {
    // 1. Load and validate settings
    ValidationResult vr;
    ModSettings settings = SettingsManager::LoadAndValidate(vr);

    if (!vr.ok) {
        LOG_WARN(L"[LiveRefresh] Settings validation failed: %ls",
                 vr.message.c_str());
        // We continue with the (possibly partially invalid) settings rather
        // than refusing to apply anything.  PropertyEngine clamps values.
    }

    // 2. Apply profile if set
    if (!settings.profile.empty() && settings.profile != L"Custom") {
        SettingsManager::ApplyProfile(settings.profile, settings);
    }

    // 3. Discover taskbar elements
    auto elements = TaskbarDiscovery::DiscoverAll();
    if (!elements.has_value()) {
        LOG_WARN(L"[LiveRefresh] ApplyNow: taskbar elements not yet available; "
                 L"will retry on next trigger.");
        return;
    }

    // 4. Apply settings to every discovered button
    int applied = 0;
    for (auto& button : elements->taskListButtons) {
        PropertyEngine::ApplyToButtonSubtree(button, settings);
        ++applied;
    }

    LOG_INFO(L"[LiveRefresh] ApplyNow complete: applied to %d buttons.", applied);
}

} // namespace TaskbarCustomizer
