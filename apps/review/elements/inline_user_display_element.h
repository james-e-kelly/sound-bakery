#pragma once

#include "pch.h"

class inline_user_display_element : public gluten::element
{
public:
    inline_user_display_element() : element(element::anchor_preset::stretch_full) {}

protected:
    auto render_element(const ImRect& parentRect) -> bool override;
};