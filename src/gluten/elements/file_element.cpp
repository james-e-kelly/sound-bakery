#include "file_element.h"

namespace gluten
{
    file_element::file_element(const anchor_preset& anchorPreset, const std::filesystem::path& filePath)
        : element(anchorPreset), m_filePath(filePath)
    {
        set_element_padding(ImVec2(0.0, 16.0f));
    }

    auto file_element::pre_render_element() -> void
    {
        ZoneScoped;

        ImGui::PushID(m_filePath.string().c_str());

        m_filePosition = get_file_play_position();
        m_fileDuration = get_file_duration();
        m_filePercent  = m_fileDuration > 0.0 ? m_filePosition / m_fileDuration : 0.0;
    }

    auto file_element::post_render_element() -> void
    {
        ImGui::PopID();
    }

    auto file_element::handle_mouse_controls(const ImRect& contentArea) -> void
    {
        if (ImGui::IsWindowFocused() && ImGui::IsMouseHoveringRect(contentArea.Min, contentArea.Max))
        {
            const bool clicked       = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
            const bool doubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
            const bool dragging      = ImGui::IsMouseDragging(ImGuiMouseButton_Left);

            if (doubleClicked)
            {
                play_file();
            }
            else if (dragging)
            {
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                if (get_is_playing())
                {
                    pause_file();
                }
                else
                {
                    play_file();
                }
            }
        }
    }

    auto file_element::handle_keyboard_controls(const ImRect& contentArea) -> void
    {
        if (ImGui::IsWindowFocused() && ImGui::IsMouseHoveringRect(contentArea.Min, contentArea.Max))
        {
            if (ImGui::IsKeyPressed(ImGuiKey_Space))
            {
                if (get_is_playing())
                {
                    pause_file();
                }
                else
                {
                    play_file();
                }
            }
        }
    }
}  // namespace gluten
