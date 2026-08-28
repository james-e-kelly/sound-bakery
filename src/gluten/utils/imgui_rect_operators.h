#pragma once

#include "imgui.h"

inline ImRect operator+(const ImRect& lhs, const ImVec2& rhs) { return ImRect(lhs.Min + rhs, lhs.Max + rhs); }

inline ImRect operator*(const ImRect& lhs, const float rhs)
{
    ImVec2 expandSize = (lhs.GetSize() * rhs) - lhs.GetSize();
    ImRect result(lhs);
    result.Expand(expandSize);
    return result;
}
