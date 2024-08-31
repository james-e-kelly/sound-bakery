#include "project_explorer_widget.h"

#include "app/app.h"
#include "gluten/utils/imgui_util_structures.h"
#include "imgui.h"
#include "managers/project_manager.h"
#include "widgets/file_browser_widget.h"
#include "widgets/project_nodes_widget.h"

void project_explorer_widget::start()
{
    widget::start();

    m_fileBrowserWidget  = add_child_widget<file_browser_widget>();
    m_projectNodesWidget = add_child_widget<project_nodes_widget>();
}

void project_explorer_widget::render()
{
    gluten::imgui::scoped_font audioFont(get_app()->get_font(gluten::fonts::regular_audio_icons));

    // Style change to make the tree/table flush with the tab buttons
    const ImVec2 currentItemSpacing = ImGui::GetStyle().ItemSpacing;
    gluten::imgui::scoped_style noYItemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(currentItemSpacing.x, 0));

    if (ImGui::Begin("Project Explorer"))
    {
        if (ImGui::BeginTabBar("Project Explorer Tabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Audio"))
            {
                gluten::imgui::scoped_style resetItemSpacing(ImGuiStyleVar_ItemSpacing, currentItemSpacing);

                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1, 1, 1, 1));

                if (m_projectNodesWidget)
                {
                    m_projectNodesWidget->render_page({SB_CATEGORY_SOUND});
                }
                ImGui::EndTabItem();

                ImGui::PopStyleColor();
            }
            if (ImGui::BeginTabItem("Nodes"))
            {
                gluten::imgui::scoped_style resetItemSpacing(ImGuiStyleVar_ItemSpacing, currentItemSpacing);

                if (m_projectNodesWidget)
                {
                    m_projectNodesWidget->render_objects_page();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Events"))
            {
                gluten::imgui::scoped_style resetItemSpacing(ImGuiStyleVar_ItemSpacing, currentItemSpacing);

                if (m_projectNodesWidget)
                {
                    m_projectNodesWidget->render_events_page();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("SoundBanks"))
            {
                gluten::imgui::scoped_style resetItemSpacing(ImGuiStyleVar_ItemSpacing, currentItemSpacing);

                if (m_projectNodesWidget)
                {
                    m_projectNodesWidget->render_soundbank_page();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    ImGui::End();
}
