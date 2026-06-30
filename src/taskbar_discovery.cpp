// =============================================================================
// taskbar_discovery.cpp — Taskbar Button Customizer
// Implementation of TaskbarDiscovery.
//
// HWND → XAML element chain:
//   Shell_TrayWnd
//     └─ Windows.UI.Composition.DesktopWindowContentBridge   (XAML Island)
//          └─ DesktopWindowXamlSource  (IDesktopWindowXamlSourceNative)
//               └─ XAML visual tree
//                    └─ Taskbar.TaskListButton (×N)
// =============================================================================
#include "taskbar_discovery.h"
#include "xaml_utils.h"
#include "logger.h"

#include <windows.h>
#include <windhawk_api.h>

#undef GetCurrentTime
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.Xaml.Media.h>

// For IDesktopWindowXamlSourceNative
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>

using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Hosting;
using namespace winrt::Windows::UI::Xaml::Media;

namespace TaskbarCustomizer {

// ── Statics ───────────────────────────────────────────────────────────────────

TaskbarDiscovery::ButtonFoundCallback TaskbarDiscovery::s_buttonFoundCallback;

/// Known XAML Island window class names across Windows 11 23H2 / 24H2 / 25H2.
const wchar_t* const TaskbarDiscovery::kXamlIslandClasses[] = {
    L"Windows.UI.Composition.DesktopWindowContentBridge",
    L"Microsoft.UI.Content.DesktopChildSiteBridge",
    nullptr
};

// ── SetButtonFoundCallback ────────────────────────────────────────────────────

void TaskbarDiscovery::SetButtonFoundCallback(ButtonFoundCallback cb) noexcept {
    s_buttonFoundCallback = std::move(cb);
}

// ── FindTaskbarHwnd ───────────────────────────────────────────────────────────

HWND TaskbarDiscovery::FindTaskbarHwnd() noexcept {
    HWND hwnd = ::FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!hwnd) {
        LOG_WARN(L"[TaskbarDiscovery] Shell_TrayWnd not found.");
    } else {
        LOG_DEBUG(L"[TaskbarDiscovery] Found Shell_TrayWnd: 0x%p", hwnd);
    }
    return hwnd;
}

// ── FindXamlIslandHwnd ────────────────────────────────────────────────────────

HWND TaskbarDiscovery::FindXamlIslandHwnd(HWND taskbarHwnd) noexcept {
    if (!taskbarHwnd) return nullptr;

    // Try each known class name; the correct one depends on the Windows build.
    for (int i = 0; kXamlIslandClasses[i]; ++i) {
        HWND island = ::FindWindowExW(taskbarHwnd, nullptr,
                                      kXamlIslandClasses[i], nullptr);
        if (island) {
            LOG_DEBUG(L"[TaskbarDiscovery] Found XAML Island '%ls': 0x%p",
                      kXamlIslandClasses[i], island);
            return island;
        }
    }

    // Fallback: enumerate ALL children and look for the class names.
    struct EnumCtx {
        HWND result = nullptr;
        const wchar_t* const* classNames = nullptr;
    } ctx;
    ctx.classNames = kXamlIslandClasses;

    ::EnumChildWindows(taskbarHwnd, [](HWND hwnd, LPARAM lp) -> BOOL {
        auto* pCtx = reinterpret_cast<EnumCtx*>(lp);
        wchar_t cls[256] = {};
        ::GetClassNameW(hwnd, cls, _countof(cls));
        for (int i = 0; pCtx->classNames[i]; ++i) {
            if (::wcscmp(cls, pCtx->classNames[i]) == 0) {
                pCtx->result = hwnd;
                return FALSE; // stop enumeration
            }
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&ctx));

    if (ctx.result) {
        LOG_DEBUG(L"[TaskbarDiscovery] Found XAML Island via EnumChildWindows: 0x%p",
                  ctx.result);
    } else {
        LOG_WARN(L"[TaskbarDiscovery] XAML Island HWND not found in taskbar children.");
    }
    return ctx.result;
}

// ── GetXamlRootFromHwnd ───────────────────────────────────────────────────────

UIElement TaskbarDiscovery::GetXamlRootFromHwnd(HWND xamlIslandHwnd) noexcept {
    if (!xamlIslandHwnd) return nullptr;

    try {
        // Retrieve IDesktopWindowXamlSourceNative from the HWND.
        // This COM interface lets us go from HWND → DesktopWindowXamlSource.
        winrt::com_ptr<IDesktopWindowXamlSourceNative> nativeSrc;
        HRESULT hr = ::SendMessageW(
            xamlIslandHwnd,
            WM_GETOBJECT,
            0,
            reinterpret_cast<LPARAM>(
                static_cast<IDesktopWindowXamlSourceNative**>(nativeSrc.put())));

        // WM_GETOBJECT is not the right approach; instead use IXamlBridge:
        // We use the standard approach: find the DesktopWindowXamlSource
        // associated with this HWND via the hosting API.
        //
        // On Windows 11 23H2+ the IDesktopWindowXamlSourceNative2 approach:
        auto sources = DesktopWindowXamlSource::GetForCurrentView();
        // sources is a IVector<DesktopWindowXamlSource>

        for (auto&& src : sources) {
            try {
                auto native = src.as<IDesktopWindowXamlSourceNative>();
                HWND srcHwnd = nullptr;
                if (SUCCEEDED(native->get_WindowHandle(&srcHwnd)) &&
                    srcHwnd == xamlIslandHwnd)
                {
                    auto root = src.Content();
                    if (root) {
                        LOG_INFO(L"[TaskbarDiscovery] Got XAML root from "
                                 L"DesktopWindowXamlSource.");
                        return root;
                    }
                }
            } catch (...) {}
        }

        LOG_WARN(L"[TaskbarDiscovery] Could not match HWND to any "
                 L"DesktopWindowXamlSource.");
        return nullptr;
    } catch (const winrt::hresult_error& e) {
        LOG_ERROR(L"[TaskbarDiscovery] GetXamlRootFromHwnd: 0x%08X %ls",
                  (unsigned)e.code().value, e.message().c_str());
        return nullptr;
    } catch (...) {
        LOG_ERROR(L"[TaskbarDiscovery] GetXamlRootFromHwnd: unknown exception");
        return nullptr;
    }
}

// ── CollectTaskListButtons ────────────────────────────────────────────────────

std::vector<DependencyObject>
TaskbarDiscovery::CollectTaskListButtons(const UIElement& root) noexcept {
    if (!root) return {};

    std::vector<DependencyObject> found;
    try {
        DependencyObject rootDO = root.as<DependencyObject>();

        XamlUtils::WalkVisualTree(rootDO, [&](DependencyObject node) -> bool {
            // Match both Taskbar.TaskListButton and
            //            Taskbar.TaskListLabeledButtonPanel
            // (the latter appears when "combine taskbar buttons + labels" is on)
            bool isButton =
                XamlUtils::TypeNameEndsWith(node, L"TaskListButton") ||
                XamlUtils::TypeNameEndsWith(node, L"TaskListLabeledButtonPanel");

            if (isButton) {
                found.push_back(node);
                LOG_DEBUG(L"[TaskbarDiscovery] Found: %ls",
                          XamlUtils::GetTypeName(node).c_str());

                // Fire callback if registered
                if (s_buttonFoundCallback) {
                    s_buttonFoundCallback(node);
                }

                // Don't descend into this button's children; we handle them
                // ourselves in PropertyEngine.
                return false; // stop descending here; sibling walk continues
            }
            return true;
        });

        LOG_INFO(L"[TaskbarDiscovery] Collected %zu taskbar button elements.",
                 found.size());
    } catch (const winrt::hresult_error& e) {
        LOG_ERROR(L"[TaskbarDiscovery] CollectTaskListButtons: 0x%08X %ls",
                  (unsigned)e.code().value, e.message().c_str());
    } catch (...) {
        LOG_ERROR(L"[TaskbarDiscovery] CollectTaskListButtons: unknown exception");
    }
    return found;
}

// ── DiscoverAll ───────────────────────────────────────────────────────────────

std::optional<TaskbarElements> TaskbarDiscovery::DiscoverAll() noexcept {
    TaskbarElements result;

    // Step 1: find the taskbar HWND
    result.taskbarHwnd = FindTaskbarHwnd();
    if (!result.taskbarHwnd) {
        LOG_WARN(L"[TaskbarDiscovery] DiscoverAll: no taskbar HWND.");
        return std::nullopt;
    }

    // Step 2: find the XAML Island HWND
    HWND islandHwnd = FindXamlIslandHwnd(result.taskbarHwnd);
    if (!islandHwnd) {
        LOG_WARN(L"[TaskbarDiscovery] DiscoverAll: no XAML Island HWND.");
        return std::nullopt;
    }

    // Step 3: obtain the XAML root
    result.xamlRoot = GetXamlRootFromHwnd(islandHwnd);
    if (!result.xamlRoot) {
        LOG_WARN(L"[TaskbarDiscovery] DiscoverAll: could not get XAML root.");
        return std::nullopt;
    }

    // Step 4: collect button elements
    result.taskListButtons = CollectTaskListButtons(result.xamlRoot);
    if (result.taskListButtons.empty()) {
        // Not necessarily fatal — taskbar may still be rendering; caller retries.
        LOG_WARN(L"[TaskbarDiscovery] DiscoverAll: no task buttons found yet.");
        return std::nullopt;
    }

    LOG_INFO(L"[TaskbarDiscovery] DiscoverAll succeeded: %zu buttons.",
             result.taskListButtons.size());
    return result;
}

} // namespace TaskbarCustomizer
