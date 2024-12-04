#include "details_widget.h"

#include "app/app.h"
#include "gluten/theme/carbon_theme_g100.h"
#include "gluten/utils/imgui_util_functions.h"
#include "gluten/utils/imgui_util_structures.h"
#include "imgui.h"
#include "managers/project_manager.h"
#include "utils/method_drawer.h"
#include "utils/property_drawer.h"
#include "elements/add_effect_button.h"

void details_widget::render()
{
    widget::render();

    const gluten::imgui::scoped_style noItemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    const gluten::imgui::scoped_style headerBorder(ImGuiStyleVar_FrameBorderSize, 1.0f);
    const gluten::imgui::scoped_color borderIsLayer1(ImGuiCol_Border, gluten::theme::carbon_g100::layer01);
    const gluten::imgui::scoped_color borderShadowIsLayer1(ImGuiCol_BorderShadow, gluten::theme::carbon_g100::layer01);
    const gluten::imgui::scoped_color frameBackgroundIsDark(ImGuiCol_FrameBg, gluten::theme::carbon_g100::background);

    if (ImGui::Begin("Details", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (std::shared_ptr<project_manager> projectManager = get_app()->get_manager_by_class<project_manager>())
        {
            if (ImGui::CollapsingHeader("Details", ImGuiTreeNodeFlags_DefaultOpen))
            {
                selection& selection = projectManager->get_selection();
                if (sbk::core::object* selected = selection.get_selected())
                {
                    property_drawer::draw_object(selected->getType(), selected);
                    method_drawer::draw_object(selected->getType(), selected);

                    add_effect_button addEffectButton;
                    addEffectButton.render_cursor();
                }
            }
        }
    }

    ImGui::End();
}