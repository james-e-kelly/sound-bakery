#include "theme.h"

namespace gluten::theme
{
    ImVec4 missingColorColor = ImVec4(0.9f, 0.4f, 0.5f, 1.0f);

    ImVec4 white = hex_to_imgui_imvec4(0xffffff);
    ImVec4 black = hex_to_imgui_imvec4(0x000000);

    ImVec4 backgroundLow                = missingColorColor;
    ImVec4 background                   = missingColorColor;
    ImVec4 backgroundInverse            = missingColorColor;
    ImVec4 backgroundActive             = missingColorColor;
    ImVec4 backgroundHover              = missingColorColor;
    ImVec4 backgroundInverseHover       = missingColorColor;
    ImVec4 backgroundSelected           = missingColorColor;
    ImVec4 backgroundSelectedHover      = missingColorColor;
    ImVec4 layer01                      = missingColorColor;
    ImVec4 layerActive01                = missingColorColor;
    ImVec4 layerHover01                 = missingColorColor;
    ImVec4 layerSelected01              = missingColorColor;
    ImVec4 layerSelectedHover01         = missingColorColor;
    ImVec4 layer02                      = missingColorColor;
    ImVec4 layerActive02                = missingColorColor;
    ImVec4 layerHover02                 = missingColorColor;
    ImVec4 layerSelected02              = missingColorColor;
    ImVec4 layerSelectedHover02         = missingColorColor;
    ImVec4 layer03                      = missingColorColor;
    ImVec4 layerActive03                = missingColorColor;
    ImVec4 layerHover03                 = missingColorColor;
    ImVec4 layerSelected03              = missingColorColor;
    ImVec4 layerSelectedHover03         = missingColorColor;
    ImVec4 layerSelectedInverse         = missingColorColor;
    ImVec4 layerSelectedDisabled        = missingColorColor;
    ImVec4 layerAccent01                = missingColorColor;
    ImVec4 layerAccentActive01          = missingColorColor;
    ImVec4 layerAccentHover01           = missingColorColor;
    ImVec4 layerAccent02                = missingColorColor;
    ImVec4 layerAccentActive02          = missingColorColor;
    ImVec4 layerAccentHover02           = missingColorColor;
    ImVec4 layerAccent03                = missingColorColor;
    ImVec4 layerAccentActive03          = missingColorColor;
    ImVec4 layerAccentHover03           = missingColorColor;
    ImVec4 field01                      = missingColorColor;
    ImVec4 fieldHover01                 = missingColorColor;
    ImVec4 field02                      = missingColorColor;
    ImVec4 fieldHover02                 = missingColorColor;
    ImVec4 field03                      = missingColorColor;
    ImVec4 fieldHover03                 = missingColorColor;
    ImVec4 borderSubtle00               = missingColorColor;
    ImVec4 borderSubtle01               = missingColorColor;
    ImVec4 borderSubtleSelected01       = missingColorColor;
    ImVec4 borderSubtle02               = missingColorColor;
    ImVec4 borderSubtleSelected02       = missingColorColor;
    ImVec4 borderSubtle03               = missingColorColor;
    ImVec4 borderSubtleSelected03       = missingColorColor;
    ImVec4 borderStrong01               = missingColorColor;
    ImVec4 borderStrong02               = missingColorColor;
    ImVec4 borderStrong03               = missingColorColor;
    ImVec4 borderTile01                 = missingColorColor;
    ImVec4 borderTile02                 = missingColorColor;
    ImVec4 borderTile03                 = missingColorColor;
    ImVec4 borderInverse                = missingColorColor;
    ImVec4 borderInteractive            = missingColorColor;
    ImVec4 borderDisabled               = missingColorColor;
    ImVec4 textPrimary                  = missingColorColor;
    ImVec4 textSecondary                = missingColorColor;
    ImVec4 textPlaceholder              = missingColorColor;
    ImVec4 textHelper                   = missingColorColor;
    ImVec4 textError                    = missingColorColor;
    ImVec4 textInverse                  = missingColorColor;
    ImVec4 textOnColor                  = missingColorColor;
    ImVec4 textOnColorDisabled          = missingColorColor;
    ImVec4 textDisabled                 = missingColorColor;
    ImVec4 linkPrimary                  = missingColorColor;
    ImVec4 linkPrimaryHover             = missingColorColor;
    ImVec4 linkSecondary                = missingColorColor;
    ImVec4 linkInverse                  = missingColorColor;
    ImVec4 linkVisited                  = missingColorColor;
    ImVec4 linkInverseActive            = missingColorColor;
    ImVec4 linkInverseHover             = missingColorColor;
    ImVec4 iconPrimary                  = missingColorColor;
    ImVec4 iconSecondary                = missingColorColor;
    ImVec4 iconInverse                  = missingColorColor;
    ImVec4 iconOnColor                  = missingColorColor;
    ImVec4 iconOnColorDisabled          = missingColorColor;
    ImVec4 iconDisabled                 = missingColorColor;
    ImVec4 iconInteractive              = missingColorColor;
    ImVec4 supportError                 = missingColorColor;
    ImVec4 supportSuccess               = missingColorColor;
    ImVec4 supportWarning               = missingColorColor;
    ImVec4 supportInfo                  = missingColorColor;
    ImVec4 focus                        = missingColorColor;
    ImVec4 focusInset                   = missingColorColor;
    ImVec4 focusInverse                 = missingColorColor;
    ImVec4 interactive                  = missingColorColor;
    ImVec4 interactiveHover             = missingColorColor;
    ImVec4 interactiveActive            = missingColorColor;
    ImVec4 interactiveSecondary         = missingColorColor;
    ImVec4 interactiveSecondaryHover    = missingColorColor;
    ImVec4 highlight                    = missingColorColor;
    ImVec4 overlay                      = missingColorColor;
    ImVec4 toggleOff                    = missingColorColor;
    ImVec4 shadow                       = missingColorColor;

    float layoutTitleBarHeight = 32.0f;
    float layoutIconRailRatio  = 1.36f;
    float layoutIconRailSize   = 48.0f;

    namespace
    {
        constexpr layer_level s_defaultLayer = layer_level::layer01;

        auto layer_stack() -> std::vector<layer_level>&
        {
            static std::vector<layer_level> stack;
            return stack;
        }
    }  // namespace

    auto current_layer() -> layer_level { return pushed_layer().value_or(s_defaultLayer); }

    auto pushed_layer() -> std::optional<layer_level>
    {
        const std::vector<layer_level>& stack = layer_stack();
        return stack.empty() ? std::nullopt : std::optional<layer_level>(stack.back());
    }

    auto resolve_layer(layer_level level) -> layer_level
    {
        if (level != layer_level::next)
        {
            return level;
        }

        switch (current_layer())
        {
            case layer_level::background:
                return layer_level::layer01;
            case layer_level::layer01:
                return layer_level::layer02;
            case layer_level::layer02:
            case layer_level::layer03:
            case layer_level::next:
                return layer_level::layer03;
        }
        return layer_level::layer03;
    }

    // Carbon pairs each surface with the next field and border-strong token up
    // (field-01 sits on the background, field-02 on layer-01, ...), while hover,
    // selection and border-subtle tokens share the surface's own index.
    auto colors_for(layer_level level) -> layer_colors
    {
        switch (resolve_layer(level))
        {
            case layer_level::background:
                return {background,     backgroundHover, backgroundActive, backgroundSelected, backgroundSelectedHover,
                        field01,        fieldHover01,    borderSubtle00,   borderStrong01,
                        layerAccent01,  layerAccentHover01, layerAccentActive01};
            case layer_level::layer01:
                return {layer01,        layerHover01,    layerActive01,    layerSelected01,    layerSelectedHover01,
                        field02,        fieldHover02,    borderSubtle01,   borderStrong02,
                        layerAccent01,  layerAccentHover01, layerAccentActive01};
            case layer_level::layer02:
                return {layer02,        layerHover02,    layerActive02,    layerSelected02,    layerSelectedHover02,
                        field03,        fieldHover03,    borderSubtle02,   borderStrong03,
                        layerAccent02,  layerAccentHover02, layerAccentActive02};
            case layer_level::layer03:
            case layer_level::next:
                return {layer03,        layerHover03,    layerActive03,    layerSelected03,    layerSelectedHover03,
                        field03,        fieldHover03,    borderSubtle03,   borderStrong03,
                        layerAccent03,  layerAccentHover03, layerAccentActive03};
        }
        return colors_for(layer_level::layer01);
    }

    auto current_colors() -> layer_colors { return colors_for(current_layer()); }

    auto push_layer(layer_level level) -> void { layer_stack().push_back(resolve_layer(level)); }

    auto pop_layer() -> void
    {
        std::vector<layer_level>& stack = layer_stack();
        IM_ASSERT(!stack.empty() && "pop_layer called without a matching push_layer");
        if (!stack.empty())
        {
            stack.pop_back();
        }
    }
}  // namespace gluten::theme