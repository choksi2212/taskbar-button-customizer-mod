// =============================================================================
// property_engine.h — Taskbar Button Customizer
// Declares PropertyEngine: the component responsible for translating a
// ModSettings snapshot into concrete XAML dependency-property writes.
//
// Each Apply* method targets a specific class of XAML elements.  All methods
// are no-throw; errors are logged and silently skipped.
// =============================================================================
#pragma once

#undef GetCurrentTime
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Media.h>

#include "settings.h"

namespace TaskbarCustomizer {

// ---------------------------------------------------------------------------
// PropertyEngine — stateless, all methods are static.
// ---------------------------------------------------------------------------
class PropertyEngine {
public:
    PropertyEngine() = delete;

    // ── Primary entry-point ─────────────────────────────────────────────────
    /// Apply ALL settings to a single TaskListButton subtree.
    /// `root` should be a Taskbar.TaskListButton or
    ///         Taskbar.TaskListLabeledButtonPanel element.
    static void ApplyToButtonSubtree(
        const winrt::Windows::UI::Xaml::DependencyObject& root,
        const ModSettings& settings) noexcept;

    // ── Individual appliers ─────────────────────────────────────────────────
    /// Resize the outer TaskListButton container (Width / Height / MinWidth /
    /// MaxWidth).
    static void ApplyButtonGeometry(
        const winrt::Windows::UI::Xaml::FrameworkElement& button,
        const ModSettings& s) noexcept;

    /// Apply CornerRadius, Padding, Margin to the BackgroundElement border.
    static void ApplyButtonAppearance(
        const winrt::Windows::UI::Xaml::Controls::Border& background,
        const ModSettings& s) noexcept;

    /// Apply Opacity to the BackgroundElement.
    static void ApplyBackgroundOpacity(
        const winrt::Windows::UI::Xaml::Controls::Border& background,
        const ModSettings& s) noexcept;

    /// Resize the icon image / animated player.
    static void ApplyIconProperties(
        const winrt::Windows::UI::Xaml::FrameworkElement& iconElement,
        const ModSettings& s) noexcept;

    /// Resize and reposition the running indicator rectangle.
    static void ApplyIndicatorProperties(
        const winrt::Windows::UI::Xaml::Shapes::Rectangle& indicator,
        const ModSettings& s) noexcept;

    /// Apply margin-based spacing between buttons.
    static void ApplySpacing(
        const winrt::Windows::UI::Xaml::FrameworkElement& button,
        const ModSettings& s) noexcept;

    // ── Restore helpers ─────────────────────────────────────────────────────
    /// Clear all mod-owned property overrides from a button subtree, restoring
    /// Windows defaults.  Called during mod uninit and graceful error recovery.
    static void RestoreButtonSubtree(
        const winrt::Windows::UI::Xaml::DependencyObject& root) noexcept;

private:
    // Cached DP lookups (populated lazily, thread-safe via static init)
    static winrt::Windows::UI::Xaml::DependencyProperty s_widthDP;
    static winrt::Windows::UI::Xaml::DependencyProperty s_heightDP;
    static winrt::Windows::UI::Xaml::DependencyProperty s_minWidthDP;
    static winrt::Windows::UI::Xaml::DependencyProperty s_maxWidthDP;
    static winrt::Windows::UI::Xaml::DependencyProperty s_cornerRadiusDP;
    static winrt::Windows::UI::Xaml::DependencyProperty s_paddingDP;
    static winrt::Windows::UI::Xaml::DependencyProperty s_marginDP;
    static winrt::Windows::UI::Xaml::DependencyProperty s_opacityDP;
    static winrt::Windows::UI::Xaml::DependencyProperty s_borderThicknessDP;
};

} // namespace TaskbarCustomizer
