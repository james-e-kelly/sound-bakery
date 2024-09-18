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
        auto render_element(const ImRect& elementBox) -> bool override;
	};
}