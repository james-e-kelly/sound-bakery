#include "pch.h"

class file_drop_element : public gluten::element
{
public:
    file_drop_element() : gluten::element(anchor_preset::left_top) {}
    file_drop_element(const std::string& displayText) : gluten::element(anchor_preset::left_top), m_displayText(displayText) {}

    std::unordered_set<std::filesystem::path> m_droppedFiles;

protected:
    auto get_element_content_size() -> ImVec2 const override
    {
        const float xSize = ImGui::GetWindowSize().x - (ImGui::GetCurrentWindow()->WindowPadding.x * 2.0f);
        return ImVec2(xSize, 100);
    }

    auto render_element(const ImRect& elementRect) -> bool override
    {
        gluten::imgui::scoped_color borderColor(ImGuiCol_Border, gluten::theme::carbon_g100::textHelper);

        gluten::background background;
        background
            .set_element_border(2.0f, 5.0f)
            .set_element_background_color(gluten::theme::carbon_g100::field02)
            .set_element_hover_color(gluten::theme::carbon_g100::fieldHover02);

        bool dropped = false;

        if (is_target_hovered(get_element_rect()))
        {
            std::unordered_set<std::filesystem::path> files = review_app::get()->get_drag_drop_files();
            std::unordered_set<std::filesystem::path> toRemove;

            if (!files.empty())
            {
                dropped = true;

                for (const auto& file : files)
                {
                    if (std::filesystem::is_directory(file))
                    {
                        for (auto iter : std::filesystem::recursive_directory_iterator(file))
                        {
                            if (iter.is_regular_file())
                            {
                                if (!files.contains(iter.path()))
                                {
                                    files.insert(iter.path());
                                }
                            }
                        }

                        toRemove.insert(file);
                    }
                }

                for (const auto& removeFile : toRemove)
                {
                    files.erase(removeFile);
                }

                m_droppedFiles = files;
            }
        }

        gluten::text text(m_displayText.empty() ? "Drop Here" : m_displayText.c_str(), ImVec2(0.5f, 0.5f),
                          anchor_preset::center_middle);

        background.render(elementRect);
        text.render(elementRect);

        ImVec2 elementEnd = get_element_rect_local().GetBL();
        elementEnd.y += ImGui::GetCurrentContext()->Style.FramePadding.y;
        
        ImGui::SetCursorPos(elementEnd);

        return dropped;
    }

private:
    auto is_target_hovered(const ImRect& bb) -> bool const
    {
        return ImGui::IsMouseHoveringRect(bb.Min, bb.Max);
    }

    std::string m_displayText;
};