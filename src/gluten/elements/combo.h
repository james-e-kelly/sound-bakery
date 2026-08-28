#pragma once

#include "gluten/elements/element.h"

#include <span>
#include <string>

namespace gluten
{
    class combo : public element
    {
    public:
        combo() = delete;
        combo(const char* id, std::span<const char* const> options, int* selectedIndex);

        auto set_button_style(button_style style) -> combo&;

        bool render_element(const element_render_info& renderInfo) override;
        auto get_element_content_size(const ImVec2& parentSize = ImVec2(0, 0)) -> ImVec2 const override;

    private:
        const char* m_id = nullptr;
        std::span<const char* const> m_options;
        int* m_selectedIndex = nullptr;
        std::optional<button_style> m_buttonStyle;
    };
}  // namespace gluten
