#pragma once

#include <rttr/type>

// Draws RTTR methods to ImGui
class MethodDrawer final
{
public:
    static void DrawObject(rttr::type type, rttr::instance instance);
};
