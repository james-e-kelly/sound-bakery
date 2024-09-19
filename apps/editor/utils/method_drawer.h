#pragma once

#include <rttr/type>

// Draws RTTR methods to ImGui
class method_drawer final
{
public:
    static void draw_object(rttr::type type, rttr::instance instance);
};
