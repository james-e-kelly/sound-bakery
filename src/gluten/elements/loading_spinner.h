#pragma once

#include "gluten/pch.h"

#include "gluten/elements/element.h"

namespace gluten
{
	class loading_spinner : public element
	{
    public:
        loading_spinner() : element(anchor_preset::stretch_full) {}

	protected:
        auto render_element(const element_render_info& renderInfo) -> bool override;
	};
}