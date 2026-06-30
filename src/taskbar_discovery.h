// =============================================================================
// taskbar_discovery.h — Taskbar Button Customizer
// Declares TaskbarDiscovery: locates the taskbar XAML Island HWND and the
// XAML elements within it that the Property Engine needs to modify.
//
// Discovery strategy (resilient, no binary offsets):
//   1. Enumerate HWNDs to find "Shell_TrayWnd" (main taskbar).
//   2. Find the child XAML Island window ("Windows.UI.Composition.DesktopWindowContentBridge").
//   3. Obtain the root DesktopWindowXamlSource from that HWND.
//   4. Walk the XAML visual tree to collect TaskListButton elements.
// =============================================================================
#pragma once

#include <windows.h>
#include <vector>
#include <functional>
#include <optional>

#undef GetCurrentTime
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>

#include "logger.h"

namespace TaskbarCustomizer {

// ---------------------------------------------------------------------------
// TaskbarElements — snapshot of discovered elements at one point in time.
// ---------------------------------------------------------------------------
struct TaskbarElements {
    /// The HWND of the main taskbar window (Shell_TrayWnd).
    HWND taskbarHwnd = nullptr;

    /// XAML root from the DesktopWindowXamlSource hosting the taskbar UI.
    winrt::Windows::UI::Xaml::UIElement xamlRoot = nullptr;

    /// All discovered TaskListButton or TaskListLabeledButtonPanel elements.
    std::vector<winrt::Windows::UI::Xaml::DependencyObject> taskListButtons;

    bool IsValid() const noexcept {
        return taskbarHwnd && xamlRoot && !taskListButtons.empty();
    }
};

// ---------------------------------------------------------------------------
// TaskbarDiscovery — stateless helper; all methods are static.
// ---------------------------------------------------------------------------
class TaskbarDiscovery {
public:
    TaskbarDiscovery() = delete;

    // ── Single full discovery ────────────────────────────────────────────────
    /// Attempt to discover all required taskbar elements.
    /// Returns nullopt when the taskbar is not ready yet (safe to retry).
    static std::optional<TaskbarElements> DiscoverAll() noexcept;

    // ── Component-level discovery ────────────────────────────────────────────
    /// Find the main taskbar HWND ("Shell_TrayWnd").
    static HWND FindTaskbarHwnd() noexcept;

    /// Find the XAML Island child HWND inside the taskbar HWND.
    static HWND FindXamlIslandHwnd(HWND taskbarHwnd) noexcept;

    /// Obtain the XAML root UIElement from a XAML Island HWND.
    static winrt::Windows::UI::Xaml::UIElement
    GetXamlRootFromHwnd(HWND xamlIslandHwnd) noexcept;

    /// Walk the XAML tree from `root` and collect all taskbar button elements.
    static std::vector<winrt::Windows::UI::Xaml::DependencyObject>
    CollectTaskListButtons(
        const winrt::Windows::UI::Xaml::UIElement& root) noexcept;

    // ── Callbacks ────────────────────────────────────────────────────────────
    /// Register a callback that fires whenever a new TaskListButton is found
    /// in the visual tree (used by the live-refresh subsystem).
    using ButtonFoundCallback =
        std::function<void(winrt::Windows::UI::Xaml::DependencyObject)>;

    static void SetButtonFoundCallback(ButtonFoundCallback cb) noexcept;

private:
    static ButtonFoundCallback s_buttonFoundCallback;

    /// Class names of the XAML Island window in various Windows 11 builds.
    static const wchar_t* const kXamlIslandClasses[];
};

} // namespace TaskbarCustomizer
