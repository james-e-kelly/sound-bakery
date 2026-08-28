#pragma once

#include "gluten/elements/element.h"

namespace gluten
{
    class scale_box : public element
    {
    public:
        scale_box() = default;
        scale_box(const anchor_preset& anchorPreset);

        auto set_aspect_ratio(float ratio) -> scale_box&;

        bool render_child(element* child);

    protected:
        auto render_element(const element_render_info& renderInfo) -> bool override;

    private:
        ImRect compute_child_rect(const ImRect& box) const;

        float m_aspectRatio          = 1.0f;
        element* m_pendingChild      = nullptr;
        std::optional<ImRect> m_lastBox;
    };
}  // namespace gluten
