#pragma once

#include "imgui.h"
#include "imgui_internal.h"

namespace gluten::imgui
{
    void shift_cursor_x(float distance);
    void shift_cursor_y(float distance);
    void shift_cursor(float x, float y);

    ImRect get_item_rect();
    ImRect rect_expanded(const ImRect& rect, float x, float y);
    ImRect rect_offset(const ImRect& rect, float x, float y);
    ImRect rect_offset(const ImRect& rect, ImVec2 xy);

    void render_window_outer_borders(ImGuiWindow* window);

    bool update_window_manual_resize(ImGuiWindow* window, ImVec2& newSize, ImVec2& newPosition);

    bool begin_menubar(const ImRect& barRectangle);
    void end_menubar();

    bool button_centered(const char* label, const ImVec2& size = ImVec2(0, 0));

    void draw_border(ImRect rect, float thickness = 1.0f, float rounding = 0.0f, float offsetX = 0.0f, float offsetY = 0.0f);
}