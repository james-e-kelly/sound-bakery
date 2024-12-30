#pragma once

#include "gluten/elements/element.h"

class add_effect_button : public gluten::element
{
protected:
    auto render_element(const ImRect& elementBox) -> bool override;
};