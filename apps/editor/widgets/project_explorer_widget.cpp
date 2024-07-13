#include "project_explorer_widget.h"

#include "app/app.h"
#include "managers/project_manager.h"
#include "widgets/file_browser_widget.h"
#include "widgets/project_nodes_widget.h"
#include "imgui.h"

void project_explorer_widget::start()
{
    widget::start();

    m_fileBrowserWidget  = add_child_widget<file_browser_widget>();
    m_projectNodesWidget = add_child_widget<project_nodes_widget>();
}

void project_explorer_widget::render()
{
    if (ImGui::Begin("Project Explorer"))
    {
        if (ImGui::BeginTabBar("Project Explorer Tabs", ImGuiTabBarFlags_None))
        {

            if (ImGui::BeginTabItem("Audio"))
            {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1, 1, 1, 1));
                
                if (m_projectNodesWidget)
                {
                    m_projectNodesWidget->RenderPage({SB_CATEGORY_SOUND});
                }
                ImGui::EndTabItem();

                ImGui::PopStyleColor();
            }
            if (ImGui::BeginTabItem("Nodes"))
            {
                if (m_projectNodesWidget)
                {
                    m_projectNodesWidget->RenderObjectsPage();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Events"))
            {
                if (m_projectNodesWidget)
                {
                    m_projectNodesWidget->RenderEventsPage();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("SoundBanks"))
            {
                if (m_projectNodesWidget)
                {
                    m_projectNodesWidget->RenderSoundbankPage();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();

        }

    }

    ImGui::End();
}
