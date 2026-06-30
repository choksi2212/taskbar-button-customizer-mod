// =============================================================================
// property_engine.cpp — Taskbar Button Customizer
// Implements PropertyEngine: writes XAML dependency properties for every
// customisable dimension exposed by the mod.
//
// DESIGN NOTES
// ─────────────
// • Every public method is noexcept.  All WinRT exceptions are caught here.
// • We do NOT cache DependencyProperty objects statically; we resolve them
//   on each call via the DP registration name.  This is slightly slower but
//   eliminates lifetime/initialization-order hazards entirely.
// • We use named element lookups (FindName) and type-safe casts to avoid
//   brittle integer-indexed child access.
// =============================================================================
#include "property_engine.h"
#include "xaml_utils.h"
#include "logger.h"

#undef GetCurrentTime
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>

using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Controls;
using namespace winrt::Windows::UI::Xaml::Media;
using namespace winrt::Windows::UI::Xaml::Shapes;

namespace TaskbarCustomizer {

// ── Static DP handles (resolved lazily) ──────────────────────────────────────
DependencyProperty PropertyEngine::s_widthDP          = nullptr;
DependencyProperty PropertyEngine::s_heightDP         = nullptr;
DependencyProperty PropertyEngine::s_minWidthDP       = nullptr;
DependencyProperty PropertyEngine::s_maxWidthDP       = nullptr;
DependencyProperty PropertyEngine::s_cornerRadiusDP   = nullptr;
DependencyProperty PropertyEngine::s_paddingDP        = nullptr;
DependencyProperty PropertyEngine::s_marginDP         = nullptr;
DependencyProperty PropertyEngine::s_opacityDP        = nullptr;
DependencyProperty PropertyEngine::s_borderThicknessDP= nullptr;

// ── Helpers ───────────────────────────────────────────────────────────────────

namespace {

/// Build a Thickness from symmetric horizontal/vertical values.
inline Thickness MakeThickness(double h, double v) noexcept {
    return Thickness{ h, v, h, v };
}

/// Build a CornerRadius from a single uniform radius value.
inline CornerRadius MakeCornerRadius(double r) noexcept {
    return CornerRadius{ r, r, r, r };
}

/// Safely cast a DependencyObject to FrameworkElement; returns nullptr on fail.
inline FrameworkElement AsFrameworkElement(
    const DependencyObject& obj) noexcept
{
    try { return obj.as<FrameworkElement>(); }
    catch (...) { return nullptr; }
}

/// Safely cast a DependencyObject to Border; returns nullptr on fail.
inline Border AsBorder(const DependencyObject& obj) noexcept {
    try { return obj.as<Border>(); }
    catch (...) { return nullptr; }
}

/// Safely cast a DependencyObject to Rectangle; returns nullptr on fail.
inline Rectangle AsRectangle(const DependencyObject& obj) noexcept {
    try { return obj.as<Rectangle>(); }
    catch (...) { return nullptr; }
}

/// Find a named element inside a FrameworkElement subtree.
/// Returns nullptr if the element is not found or cannot be cast.
inline DependencyObject FindNamedChild(
    const DependencyObject& root,
    const wchar_t* name) noexcept
{
    try {
        auto fe = root.as<FrameworkElement>();
        if (!fe) return nullptr;
        auto result = fe.FindName(name);
        return result ? result.as<DependencyObject>() : nullptr;
    } catch (...) {
        return nullptr;
    }
}

} // anonymous namespace

// ── ApplyToButtonSubtree ──────────────────────────────────────────────────────

void PropertyEngine::ApplyToButtonSubtree(
    const DependencyObject& root,
    const ModSettings& settings) noexcept
{
    if (!root) return;

    try {
        // 1. Outer button element (geometry + spacing)
        if (auto fe = AsFrameworkElement(root)) {
            ApplyButtonGeometry(fe, settings);
            ApplySpacing(fe, settings);
        }

        // 2. Walk subtree for named / typed elements
        XamlUtils::WalkVisualTree(root, [&](DependencyObject node) -> bool {
            // BackgroundElement — corner radius, padding, margin, opacity
            if (auto border = AsBorder(node)) {
                // Identify by automation / name check
                if (auto fe = AsFrameworkElement(node)) {
                    auto name = fe.Name();
                    if (!name.empty()) {
                        auto nameStr = std::wstring{ name };
                        if (nameStr == L"BackgroundElement" ||
                            nameStr == L"BackgroundBorder") {
                            ApplyButtonAppearance(border, settings);
                            ApplyBackgroundOpacity(border, settings);
                        }
                    }
                }
            }

            // Icon — Image or AnimatedVisualPlayer
            if (XamlUtils::TypeNameEndsWith(node, L"Image") ||
                XamlUtils::TypeNameEndsWith(node, L"AnimatedVisualPlayer"))
            {
                if (auto iconFe = AsFrameworkElement(node)) {
                    auto name = iconFe.Name();
                    auto nameStr = name.empty() ? L"" : std::wstring{ name };
                    if (nameStr == L"Icon" || nameStr == L"AppIcon" ||
                        nameStr.empty()) // unnamed icons inside button
                    {
                        ApplyIconProperties(iconFe, settings);
                    }
                }
            }

            // Running indicator rectangle
            if (auto rect = AsRectangle(node)) {
                if (auto fe = AsFrameworkElement(node)) {
                    auto name = fe.Name();
                    if (!name.empty() &&
                        std::wstring{ name } == L"RunningIndicator")
                    {
                        ApplyIndicatorProperties(rect, settings);
                    }
                }
            }

            return true; // continue walking
        });

    } catch (const winrt::hresult_error& e) {
        LOG_ERROR(L"[PropertyEngine] ApplyToButtonSubtree: 0x%08X %ls",
                  (unsigned)e.code().value, e.message().c_str());
    } catch (...) {
        LOG_ERROR(L"[PropertyEngine] ApplyToButtonSubtree: unknown exception");
    }
}

// ── ApplyButtonGeometry ───────────────────────────────────────────────────────

void PropertyEngine::ApplyButtonGeometry(
    const FrameworkElement& button,
    const ModSettings& s) noexcept
{
    try {
        // Width: 0 means "let Windows decide" → leave as NaN (auto)
        if (s.buttonWidth > 0.0) {
            button.Width(s.buttonWidth);
        } else {
            button.Width(std::numeric_limits<double>::quiet_NaN());
        }

        if (s.buttonHeight > 0.0) {
            button.Height(s.buttonHeight);
        } else {
            button.Height(std::numeric_limits<double>::quiet_NaN());
        }

        button.MinWidth (s.buttonMinWidth);
        button.MaxWidth (s.buttonMaxWidth);

        LOG_DEBUG(L"[PropertyEngine] Geometry: W=%.1f H=%.1f minW=%.1f maxW=%.1f",
                  s.buttonWidth, s.buttonHeight,
                  s.buttonMinWidth, s.buttonMaxWidth);
    } catch (const winrt::hresult_error& e) {
        LOG_WARN(L"[PropertyEngine] ApplyButtonGeometry: 0x%08X %ls",
                 (unsigned)e.code().value, e.message().c_str());
    }
}

// ── ApplyButtonAppearance ─────────────────────────────────────────────────────

void PropertyEngine::ApplyButtonAppearance(
    const Border& background,
    const ModSettings& s) noexcept
{
    try {
        background.CornerRadius(MakeCornerRadius(s.cornerRadius));
        background.Padding     (MakeThickness(s.paddingHorizontal, s.paddingVertical));
        background.Margin      (MakeThickness(s.marginHorizontal,  s.marginVertical));
        background.BorderThickness(MakeThickness(s.borderThickness, s.borderThickness));

        LOG_DEBUG(L"[PropertyEngine] Appearance: radius=%.1f "
                  L"paddH=%.1f paddV=%.1f marginH=%.1f marginV=%.1f",
                  s.cornerRadius,
                  s.paddingHorizontal, s.paddingVertical,
                  s.marginHorizontal,  s.marginVertical);
    } catch (const winrt::hresult_error& e) {
        LOG_WARN(L"[PropertyEngine] ApplyButtonAppearance: 0x%08X %ls",
                 (unsigned)e.code().value, e.message().c_str());
    }
}

// ── ApplyBackgroundOpacity ────────────────────────────────────────────────────

void PropertyEngine::ApplyBackgroundOpacity(
    const Border& background,
    const ModSettings& s) noexcept
{
    try {
        // Clamp to [0, 1] for safety even though Validate() already checked.
        double opacity = std::max(0.0, std::min(1.0, s.backgroundOpacity));
        background.Opacity(opacity);
        LOG_DEBUG(L"[PropertyEngine] BackgroundOpacity: %.2f", opacity);
    } catch (const winrt::hresult_error& e) {
        LOG_WARN(L"[PropertyEngine] ApplyBackgroundOpacity: 0x%08X %ls",
                 (unsigned)e.code().value, e.message().c_str());
    }
}

// ── ApplyIconProperties ───────────────────────────────────────────────────────

void PropertyEngine::ApplyIconProperties(
    const FrameworkElement& iconElement,
    const ModSettings& s) noexcept
{
    try {
        if (s.iconSize > 0.0) {
            iconElement.Width (s.iconSize);
            iconElement.Height(s.iconSize);
        }
        iconElement.Opacity(std::max(0.0, std::min(1.0, s.iconOpacity)));

        // Icon offset is applied via margin on the icon element.
        // Margin(left, top, right, bottom): we shift left/up by putting offset
        // on left and top.
        Thickness iconMargin {
            s.iconOffsetX, s.iconOffsetY, -s.iconOffsetX, -s.iconOffsetY
        };
        iconElement.Margin(iconMargin);

        // Scale via RenderTransform
        if (s.iconScale != 1.0) {
            winrt::Windows::UI::Xaml::Media::ScaleTransform st;
            st.ScaleX(s.iconScale);
            st.ScaleY(s.iconScale);
            iconElement.RenderTransform(st);
            iconElement.RenderTransformOrigin({ 0.5, 0.5 });
        } else {
            iconElement.RenderTransform(nullptr);
        }

        LOG_DEBUG(L"[PropertyEngine] Icon: size=%.1f opacity=%.2f "
                  L"offsetX=%.1f offsetY=%.1f scale=%.2f",
                  s.iconSize, s.iconOpacity,
                  s.iconOffsetX, s.iconOffsetY, s.iconScale);
    } catch (const winrt::hresult_error& e) {
        LOG_WARN(L"[PropertyEngine] ApplyIconProperties: 0x%08X %ls",
                 (unsigned)e.code().value, e.message().c_str());
    }
}

// ── ApplyIndicatorProperties ──────────────────────────────────────────────────

void PropertyEngine::ApplyIndicatorProperties(
    const Rectangle& indicator,
    const ModSettings& s) noexcept
{
    try {
        if (auto fe = indicator.as<FrameworkElement>()) {
            if (s.indicatorHeight > 0.0) fe.Height(s.indicatorHeight);
            if (s.indicatorWidth  > 0.0) fe.Width (s.indicatorWidth);
        }
        indicator.RadiusX(s.indicatorCornerRadius);
        indicator.RadiusY(s.indicatorCornerRadius);

        LOG_DEBUG(L"[PropertyEngine] Indicator: H=%.1f W=%.1f radius=%.1f",
                  s.indicatorHeight, s.indicatorWidth, s.indicatorCornerRadius);
    } catch (const winrt::hresult_error& e) {
        LOG_WARN(L"[PropertyEngine] ApplyIndicatorProperties: 0x%08X %ls",
                 (unsigned)e.code().value, e.message().c_str());
    }
}

// ── ApplySpacing ──────────────────────────────────────────────────────────────

void PropertyEngine::ApplySpacing(
    const FrameworkElement& button,
    const ModSettings& s) noexcept
{
    try {
        // Inter-button spacing is achieved by setting a symmetric horizontal
        // margin on the button itself.
        double half = s.spacingBetweenButtons / 2.0;
        Thickness m = button.Margin();
        m.Left   = half;
        m.Right  = half;
        button.Margin(m);

        LOG_DEBUG(L"[PropertyEngine] Spacing: between=%.1f (half=%.1f)",
                  s.spacingBetweenButtons, half);
    } catch (const winrt::hresult_error& e) {
        LOG_WARN(L"[PropertyEngine] ApplySpacing: 0x%08X %ls",
                 (unsigned)e.code().value, e.message().c_str());
    }
}

// ── RestoreButtonSubtree ──────────────────────────────────────────────────────

void PropertyEngine::RestoreButtonSubtree(
    const DependencyObject& root) noexcept
{
    if (!root) return;

    try {
        XamlUtils::WalkVisualTree(root, [](DependencyObject node) -> bool {
            try {
                // Clear Width / Height overrides on all FrameworkElements
                if (auto fe = node.try_as<FrameworkElement>()) {
                    fe.ClearValue(FrameworkElement::WidthProperty());
                    fe.ClearValue(FrameworkElement::HeightProperty());
                    fe.ClearValue(FrameworkElement::MinWidthProperty());
                    fe.ClearValue(FrameworkElement::MaxWidthProperty());
                    fe.ClearValue(FrameworkElement::MarginProperty());
                    fe.ClearValue(UIElement::OpacityProperty());
                    fe.RenderTransform(nullptr);
                }
                // Clear Border-specific properties
                if (auto border = node.try_as<Border>()) {
                    border.ClearValue(Border::CornerRadiusProperty());
                    border.ClearValue(Border::PaddingProperty());
                    border.ClearValue(Border::BorderThicknessProperty());
                }
                // Clear Rectangle radius overrides
                if (auto rect = node.try_as<Rectangle>()) {
                    rect.ClearValue(Rectangle::RadiusXProperty());
                    rect.ClearValue(Rectangle::RadiusYProperty());
                }
            } catch (...) {}
            return true;
        });

        LOG_DEBUG(L"[PropertyEngine] RestoreButtonSubtree complete.");
    } catch (...) {
        LOG_WARN(L"[PropertyEngine] RestoreButtonSubtree: exception swallowed.");
    }
}

} // namespace TaskbarCustomizer
