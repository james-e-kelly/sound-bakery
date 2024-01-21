#include "ProjectExplorerWidget.h"

#include "App/App.h"
#include "Managers/ProjectManager.h"
#include "Widgets/FileBrowserWidget.h"
#include "Widgets/ProjectNodesWidget.h"
#include "imgui.h"

void ProjectExplorerWidget::Start()
{
    Widget::Start();

    m_fileBrowserWidget  = AddChildWidget<FileBrowserWidget>();
    m_projectNodesWidget = AddChildWidget<ProjectNodesWidget>();
}

void ProjectExplorerWidget::Render()
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
                ImGui::Text("SoundBanks Go Here");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();

        }

    }

    ImGui::End();
}
