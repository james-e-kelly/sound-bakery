#include "play_controls_widget.h"

#include "app/app.h"
#include "audio_display_widget.h"
#include "imgui.h"
#include "managers/project_manager.h"
#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/event/event.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/system.h"
#include "sound_bakery/util/type_helper.h"
#include "sound_bakery/voice/node_instance.h"
#include "sound_bakery/voice/voice.h"

#include <rttr/type>

static sbk::core::object* s_lastPlayableSelection;

void player_widget::start()
{
    widget::start();

    add_child_widget<audio_display_widget>();
}

void player_widget::render()
{
    ImGui::Begin("Player");

    selection& selection                   = get_app()->get_manager_by_class<project_manager>()->get_selection();
    std::optional<rttr::type> selectedType = selection.selected_type();

    const bool isSelected = !!selection.get_selected();
    const bool isPlayable =
        isSelected && selectedType.has_value() && sbk::util::type_helper::isTypePlayable(selectedType.value());

    if (isPlayable)
    {
        // Stop playing previous if we're selecting something new that is playable
        if (selection.get_selected() != s_lastPlayableSelection)
        {
            sbk::engine::system::get()->get_listener_game_object()->stop_all();
        }

        s_lastPlayableSelection = selection.get_selected();
    }
    else if (!isSelected)
    {
        s_lastPlayableSelection = nullptr;
    }

    ImGui::BeginDisabled(!isSelected || !isPlayable);

    if (isSelected && !!s_lastPlayableSelection)
    {
        ImGui::Text(
            "%s",
            s_lastPlayableSelection->try_convert_object<sbk::core::database_object>()->get_database_name().data());
    }

    if (ImGui::IsKeyReleased(ImGuiKey_Space))
    {
        toggle_play_selected();
    }

    if (ImGui::Button("Play"))
    {
        play_selected();
    }

    ImGui::SameLine();

    if (ImGui::Button("Stop"))
    {
        stop_selected();
    }

    ImGui::EndDisabled();

    sbk::engine::node* const nodeSelection =
        s_lastPlayableSelection ? s_lastPlayableSelection->try_convert_object<sbk::engine::node>() : nullptr;

    sbk::engine::global_parameter_list parameterList;

    if (nodeSelection)
    {
        nodeSelection->gatherParameters(parameterList);
    }

    sbk::engine::game_object* const listenerGameObject = sbk::engine::system::get()->get_listener_game_object();

    if (ImGui::BeginTabBar("Runtime Parameters"))
    {
        if (ImGui::BeginTabItem("Switches"))
        {
            if (ImGui::BeginTable("Table", 2))
            {
                for (const sbk::core::database_ptr<sbk::engine::named_parameter>& intParameter :
                     parameterList.intParameters)
                {
                    if (intParameter.lookup() == false)
                    {
                        continue;
                    }

                    sbk::core::database_ptr<sbk::engine::named_parameter_value> selectedIntParameterValue =
                        listenerGameObject->get_int_parameter_value(intParameter);

                    if (selectedIntParameterValue.lookup() == false)
                    {
                        continue;
                    }

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(intParameter->get_database_name().data());

                    ImGui::TableNextColumn();

                    if (ImGui::BeginCombo("Selected", selectedIntParameterValue->get_database_name().data()))
                    {
                        for (sbk::core::database_ptr<sbk::engine::named_parameter_value> parameterValue :
                             intParameter->get_values())
                        {
                            if (parameterValue.lookup() == false)
                            {
                                continue;
                            }

                            bool selected = parameterValue == selectedIntParameterValue;
                            if (ImGui::Selectable(parameterValue->get_database_name().data(), &selected))
                            {
                                listenerGameObject->set_int_parameter_value(
                                    {intParameter.id(), parameterValue->get_database_id()});
                            }
                        }

                        ImGui::EndCombo();
                    }
                }

                ImGui::EndTable();
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    render_children();

    ImGui::End();
}

void player_widget::play_selected()
{
    if (sbk::engine::container* container = s_lastPlayableSelection->try_convert_object<sbk::engine::container>())
    {
        sbk::engine::system::get()->get_listener_game_object()->play_container(container);
    }
    else if (sbk::engine::sound* sound = s_lastPlayableSelection->try_convert_object<sbk::engine::sound>())
    {
        if (sbk::engine::sound_container* previewContainer =
                sbk::engine::system::get_project()->get_preview_container().lock().get())
        {
            previewContainer->set_sound(sound);

            sbk::engine::system::get()->get_listener_game_object()->play_container(previewContainer);
        }
    }
    else if (sbk::engine::event* event = s_lastPlayableSelection->try_convert_object<sbk::engine::event>())
    {
        sbk::engine::system::get()->get_listener_game_object()->post_event(event);
    }
}

void player_widget::stop_selected() { sbk::engine::system::get()->get_listener_game_object()->stop_all(); }

void player_widget::toggle_play_selected()
{
    if (sbk::engine::system::get()->get_listener_game_object()->is_playing())
    {
        stop_selected();
    }
    else
    {
        play_selected();
    }
}