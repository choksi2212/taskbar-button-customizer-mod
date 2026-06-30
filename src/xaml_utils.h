// =============================================================================
// xaml_utils.h — Taskbar Button Customizer
// Utilities for walking the WinRT XAML visual tree and converting between
// Win32 / WinRT / COM types.  Shared by discovery.h and property_engine.h.
// =============================================================================
#pragma once

// Pull in WinRT XAML headers.  The undef is required because WinRT defines
// GetCurrentTime as a macro that conflicts with Windows.h.
#undef GetCurrentTime
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>

#include <string>
#include <string_view>
#include <functional>
#include <optional>
#include <vector>

#include "logger.h"

namespace TaskbarCustomizer {

// ---------------------------------------------------------------------------
// XamlUtils — stateless helper namespace
// ---------------------------------------------------------------------------
namespace XamlUtils {

// ── Type name helpers ────────────────────────────────────────────────────────

/// Returns the RuntimeClass name of any IInspectable-derived WinRT object.
/// Returns an empty string on failure (never throws).
inline std::wstring GetTypeName(
    const winrt::Windows::Foundation::IInspectable& obj) noexcept
{
    try {
        if (!obj) return {};
        return std::wstring{ obj.as<winrt::Windows::UI::Xaml::DependencyObject>()
            .Dispatcher()
            .HasThreadAccess()
            ? winrt::get_class_name(obj)
            : L"<off-thread>" };
    } catch (...) {
        return {};
    }
}

/// Returns true when the last segment of obj's class name matches `shortName`.
/// e.g. "Taskbar.TaskListButton" matches "TaskListButton".
inline bool TypeNameEndsWith(
    const winrt::Windows::Foundation::IInspectable& obj,
    std::wstring_view shortName) noexcept
{
    try {
        auto name = winrt::get_class_name(obj);
        if (name.size() < shortName.size()) return false;
        return std::wstring_view{ name }.ends_with(shortName);
    } catch (...) {
        return false;
    }
}

/// Returns true when obj's full class name exactly matches `fullName`.
inline bool TypeNameIs(
    const winrt::Windows::Foundation::IInspectable& obj,
    std::wstring_view fullName) noexcept
{
    try {
        return std::wstring_view{ winrt::get_class_name(obj) } == fullName;
    } catch (...) {
        return false;
    }
}

// ── Visual tree traversal ────────────────────────────────────────────────────

/// Invoke `visitor` on every XAML element in the subtree rooted at `root`
/// (pre-order, depth-first).  If `visitor` returns false the walk stops early.
/// The root itself is also visited.
inline void WalkVisualTree(
    const winrt::Windows::UI::Xaml::DependencyObject& root,
    const std::function<bool(winrt::Windows::UI::Xaml::DependencyObject)>& visitor)
{
    if (!root) return;
    try {
        if (!visitor(root)) return;

        int count = winrt::Windows::UI::Xaml::Media::VisualTreeHelper::
                        GetChildrenCount(root);
        for (int i = 0; i < count; ++i) {
            auto child = winrt::Windows::UI::Xaml::Media::VisualTreeHelper::
                             GetChild(root, i);
            WalkVisualTree(child, visitor);
        }
    } catch (const winrt::hresult_error& e) {
        LOG_WARN(L"[XamlUtils] WalkVisualTree exception: 0x%08X %ls",
                 (unsigned)e.code().value, e.message().c_str());
    } catch (...) {
        LOG_WARN(L"[XamlUtils] WalkVisualTree unknown exception");
    }
}

/// Collect all elements in a subtree whose full class name ends with shortName.
inline std::vector<winrt::Windows::UI::Xaml::DependencyObject>
FindAllByTypeName(
    const winrt::Windows::UI::Xaml::DependencyObject& root,
    std::wstring_view shortName)
{
    std::vector<winrt::Windows::UI::Xaml::DependencyObject> results;
    WalkVisualTree(root, [&](auto node) -> bool {
        if (TypeNameEndsWith(node, shortName)) {
            results.push_back(node);
        }
        return true; // continue walking
    });
    return results;
}

/// Returns the first element in the subtree whose full class name ends with
/// `shortName`, or nullptr if not found.
inline winrt::Windows::UI::Xaml::DependencyObject
FindFirstByTypeName(
    const winrt::Windows::UI::Xaml::DependencyObject& root,
    std::wstring_view shortName)
{
    winrt::Windows::UI::Xaml::DependencyObject found;
    WalkVisualTree(root, [&](auto node) -> bool {
        if (TypeNameEndsWith(node, shortName)) {
            found = node;
            return false; // stop
        }
        return true;
    });
    return found;
}

/// Walk from `node` upward through parents until we reach a node matching
/// `ancestorTypeName` (short name check), or return nullptr.
inline winrt::Windows::UI::Xaml::DependencyObject
FindAncestorByTypeName(
    const winrt::Windows::UI::Xaml::DependencyObject& node,
    std::wstring_view ancestorTypeName)
{
    if (!node) return nullptr;
    try {
        auto current = winrt::Windows::UI::Xaml::Media::VisualTreeHelper::
                           GetParent(node);
        while (current) {
            if (TypeNameEndsWith(current, ancestorTypeName)) return current;
            current = winrt::Windows::UI::Xaml::Media::VisualTreeHelper::
                          GetParent(current);
        }
    } catch (...) {}
    return nullptr;
}

// ── Dependency property helpers ───────────────────────────────────────────────

/// Safely try to set a dependency property; returns true on success.
/// All exceptions are caught and logged; the caller is never thrown at.
template<typename T>
inline bool TrySetProperty(
    const winrt::Windows::UI::Xaml::DependencyObject& obj,
    const winrt::Windows::UI::Xaml::DependencyProperty& dp,
    T value) noexcept
{
    try {
        obj.SetValue(dp, winrt::box_value(value));
        return true;
    } catch (const winrt::hresult_error& e) {
        LOG_WARN(L"[XamlUtils] SetValue failed: 0x%08X %ls",
                 (unsigned)e.code().value, e.message().c_str());
        return false;
    } catch (...) {
        LOG_WARN(L"[XamlUtils] SetValue unknown exception");
        return false;
    }
}

/// Safely clear a dependency property back to its default value.
inline bool TryClearProperty(
    const winrt::Windows::UI::Xaml::DependencyObject& obj,
    const winrt::Windows::UI::Xaml::DependencyProperty& dp) noexcept
{
    try {
        obj.ClearValue(dp);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace XamlUtils
} // namespace TaskbarCustomizer
