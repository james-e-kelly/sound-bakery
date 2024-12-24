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

void soundbank_viewer_widget::render_implementation()
{
    static sc_sound* sound = nullptr;
    static sc_sound_instance* soundInstance = nullptr;

    if (ImGui::Begin("Soundbank Viewer"))
    {
        for (sbk::core::object* object : sbk::engine::system::get()->get_objects_of_category(SB_CATEGORY_EVENT))
        {
            if (sbk::engine::event* event = object->try_convert_object<sbk::engine::event>())
            {
                if (ImGui::Button(event->get_database_name().data()))
                {
                    sbk::engine::system::get()->get_listener_game_object()->post_event(event);
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