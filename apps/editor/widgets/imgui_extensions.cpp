#include "imgui.h"

ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) noexcept { return {lhs.x + rhs.x, lhs.y + rhs.y}; }