#include "database_widget.h"

#include "sound_bakery/system.h"

auto database_widget::render_implementation() -> void
{
    if (ImGui::Begin(get_widget_name().data()))
    {
        if (ImGui::BeginTable(
                "Database", 2,
                ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp))
        {
            std::vector<sbk::core::database_name> names = sbk::engine::system::get()->get_all_database_names();
            std::sort(names.begin(), names.end(), sbk::core::database_name_comparator());

            for (const sbk::core::database_name& name : names)
            {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(name.databaseName.c_str());
                ImGui::TableNextColumn();

                std::weak_ptr<sbk::core::database_object> foundObject =
                    sbk::engine::system::get()->try_find_database_object(name);
                if (foundObject.expired())
                {
                    ImGui::TextUnformatted("Object Not Found");
                }
                else
                {
                    ImGui::TextUnformatted(foundObject.lock()->get_asset_name().c_str());
                }
            }

            ImGui::EndTable();
        }
    }

    ImGui::End();
}