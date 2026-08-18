#include "soundbank_viewer.h"

#include "sound_bakery/event/event.h"
#include "sound_bakery/gameobject/gameobject.h"

#include "imgui.h"

void soundbank_viewer_widget::start_implementation()
{
    sbk_system_config config = sbk_system_config_init_default();

    (void)sbk::engine::system::create();
    (void)sbk::engine::system::get()->init(config);
}

void soundbank_viewer_widget::tick_implementation(double deltaTime)
{
    (void)sbk::engine::system::get()->update();
}

void soundbank_viewer_widget::render_implementation()
{
    if (ImGui::Begin(get_widget_name().data()))
    {
        for (sbk::core::object* object : sbk::engine::system::get()->get_objects_of_category(sbk::memory::object_category::event))
        {
            if (sbk::engine::event* event = object->try_convert_object<sbk::engine::event>())
            {
                if (ImGui::Button(event->get_database_name()))
                {
                    (void)sbk_system_post_event(event->get_database_id(), 0);
                }
            }
        }

        for (auto& typesToObjects : sbk::engine::system::get()->get_all_type_to_objects())
        {
            if (ImGui::CollapsingHeader(typesToObjects.first.get_name().data()))
            {
                for (auto& object : typesToObjects.second)
                {
                    if (const sbk::core::database_object* const databaseObject = object->try_convert_object<sbk::core::database_object>())
                    {
                        ImGui::TextUnformatted(databaseObject->get_database_name());
                    }
                    else
                    {
                        ImGui::TextUnformatted("No Name");
                    }
                }
            }
        }
    }

    ImGui::End();
}

void soundbank_viewer_widget::end_implementation()
{
    sbk::engine::system::destroy();
}

void soundbank_viewer_widget::set_soundbank_to_view(const std::filesystem::path& soundbankFilePath)
{
    sbk_id id;
    (void)sbk_system_load_soundbank(soundbankFilePath.string().c_str(), &id);
}