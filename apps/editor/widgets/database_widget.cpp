#include "database_widget.h"

#include "sound_bakery/system.h"

auto database_widget::render_implementation() -> void
{
    if (ImGui::Begin(get_widget_name().data(), &m_visible))
    {
        if (ImGui::BeginTable(
                "Database", 2,
                ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp))
        {
            std::vector<std::pair<sbk::core::database_name, std::string>> rows;
            for (const std::weak_ptr<sbk::core::database_object>& weakObject : sbk::engine::system::get()->get_all_database_objects())
            {
                if (const std::shared_ptr<sbk::core::database_object> object = weakObject.lock())
                {
                    rows.emplace_back(object->get_database_name(), object->get_asset_name());
                }
            }

            std::sort(rows.begin(), rows.end(),
                      [](const auto& lhs, const auto& rhs)
                      { return sbk::core::database_name_comparator()(lhs.first, rhs.first); });

            for (const auto& [name, assetName] : rows)
            {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(name.databaseName.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(assetName.c_str());
            }

            ImGui::EndTable();
        }
    }

    ImGui::End();
}