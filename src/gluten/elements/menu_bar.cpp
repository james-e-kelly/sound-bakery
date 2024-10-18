#include "menu_bar.h"

#include "gluten/theme/carbon_theme_g100.h"
#include "gluten/utils/imgui_util_structures.h"

namespace menu_bar_utils
{
    constexpr const char* menubar_id = "##menubar";
}

gluten::menu_bar::menu_bar() : element(anchor_preset::stretch_full) {}

gluten::menu_bar::~menu_bar() { end_menu_bar(); }

auto gluten::menu_bar::render_element(const ImRect& elementBox) -> bool
{
    if (ImGuiWindow* const window = ImGui::GetCurrentWindow())
    {
        if (window->SkipItems)
        {
            return false;
        }

        ImGui::BeginGroup();
        ImGui::PushID(menu_bar_utils::menubar_id);

        ImGui::PushClipRect(elementBox.Min, elementBox.Max, true);

        window->DC.LayoutType       = ImGuiLayoutType_Horizontal;
        window->DC.NavLayerCurrent  = ImGuiNavLayer_Menu;
        window->DC.MenuBarAppending = true;

        const float height         = elementBox.GetSize().y;
        const float halfHeight     = height / 2.0f;
        const float textHeight     = ImGui::GetTextLineHeight();
        const float textHalfHeight = textHeight / 2.0f;

        const float desiredY = elementBox.Min.y + halfHeight - textHalfHeight;

        ImVec2 topLeft = elementBox.GetTL();
        topLeft.y      = desiredY;
        ImGui::SetCursorScreenPos(topLeft);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, gluten::theme::carbon_g100::paddingVec);

    return true;
}

void gluten::menu_bar::end_menu_bar()
{
    if (m_hasEnded)
    {
        return;
    }

    m_hasEnded = true;

    if (ImGuiWindow* const window = ImGui::GetCurrentWindow())
    {
        if (window->SkipItems)
        {
            return;
        }

        ImGuiContext& g = *GImGui;

        // Nav: When a move request within one of our child menu failed, capture the request to navigate among our
        // siblings.
        if (ImGui::NavMoveRequestButNoResultYet() &&
            (g.NavMoveDir == ImGuiDir_Left || g.NavMoveDir == ImGuiDir_Right) &&
            (g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu))
        {
            // Try to find out if the request is for one of our child menu
            ImGuiWindow* nav_earliest_child = g.NavWindow;
            while (nav_earliest_child->ParentWindow &&
                   (nav_earliest_child->ParentWindow->Flags & ImGuiWindowFlags_ChildMenu))
                nav_earliest_child = nav_earliest_child->ParentWindow;
            if (nav_earliest_child->ParentWindow == window &&
                nav_earliest_child->DC.ParentLayoutType == ImGuiLayoutType_Horizontal &&
                (g.NavMoveFlags & ImGuiNavMoveFlags_Forwarded) == 0)
            {
                // To do so we claim focus back, restore NavId and then process the movement request for yet another
                // frame. This involve a one-frame delay which isn't very problematic in this situation. We could remove
                // it by scoring in advance for multiple window (probably not worth bothering)
                const ImGuiNavLayer layer = ImGuiNavLayer_Menu;
                IM_ASSERT(window->DC.NavLayersActiveMaskNext & (1 << layer));  // Sanity check
                ImGui::FocusWindow(window);
                ImGui::SetNavID(window->NavLastIds[layer], layer, 0, window->NavRectRel[layer]);
                g.NavCursorVisible =
                    false;  // Hide highlight for the current frame so we don't see the intermediary selection.
                g.NavHighlightItemUnderNav = g.NavMousePosDirty = true;
                ImGui::NavMoveRequestForward(g.NavMoveDir, g.NavMoveClipDir, g.NavMoveFlags,
                                             g.NavMoveScrollFlags);  // Repeat
            }
        }

        IM_MSVC_WARNING_SUPPRESS(6011);  // Static Analysis false positive "warning C6011: Dereferencing NULL pointer
                                         // 'window'"
        // IM_ASSERT(window->Flags & ImGuiWindowFlags_MenuBar); // NOTE(Yan): Needs to be commented out because Jay
        IM_ASSERT(window->DC.MenuBarAppending);
        ImGui::PopClipRect();
        ImGui::PopID();
        window->DC.MenuBarOffset.x =
            window->DC.CursorPos.x - window->Pos.x;  // Save left_to_right position so next append can reuse it. This is
                                                     // kinda equivalent to a per-layer CursorPos.
        g.GroupStack.back().EmitItem = false;
        ImGui::EndGroup();  // Restore position on layer 0
        window->DC.LayoutType       = ImGuiLayoutType_Vertical;
        window->DC.NavLayerCurrent  = ImGuiNavLayer_Main;
        window->DC.MenuBarAppending = false;
    }

    ImGui::PopStyleVar();

    ImGui::SetWindowFontScale(1.0f);
}