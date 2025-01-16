#include "soundbank_viewer.h"

#include "sound_bakery/event/event.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_chef/sound_chef_bank.h"

#include "imgui.h"

void soundbank_viewer_widget::start_implementation()
{ 
    sb_system_config config = sb_system_config_init_default();

    sbk::engine::system::create(); 
    sbk::engine::system::init(config);
}

void soundbank_viewer_widget::tick_implementation(double deltaTime)
{
    sbk::engine::system::update();
}

void soundbank_viewer_widget::render_implementation()
{
    if (ImGui::Begin("Soundbank Viewer"))
    {
        for (sbk::core::object* object : sbk::engine::system::get()->get_objects_of_category(SB_CATEGORY_EVENT))
        {
            if (sbk::engine::event* event = object->try_convert_object<sbk::engine::event>())
            {
                if (ImGui::Button(event->get_database_name().data()))
                {
                    sbk::engine::system::post_event(event->get_database_name().data(), 0);
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
                        ImGui::TextUnformatted(databaseObject->get_database_name().data());
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
    sbk::engine::system::load_soundbank(soundbankFilePath);
}