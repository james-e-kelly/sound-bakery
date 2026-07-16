#pragma once

#include "gluten/elements/layouts/layout.h"

namespace gluten
{
    class toolbar : public layout
    {
    public:
        toolbar() = default;
        toolbar(const layout_type& type);

    protected:
        auto render_element(const element_render_info& renderInfo) -> bool override;
    };
}  // namespace gluten