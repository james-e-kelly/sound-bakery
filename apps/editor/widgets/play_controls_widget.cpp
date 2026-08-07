#include "play_controls_widget.h"

#include "sound_bakery/api/engine_api.h"
#include "sound_bakery/editor/project/project.h"
#include "sound_bakery/event/event.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/runtime/runtime.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/system.h"
#include "sound_bakery/util/type_helper.h"
#include "sound_bakery/voice/node_instance.h"
#include "sound_bakery/voice/voice.h"

#include "IconsLucide.h"
#include "app/app.h"
#include "audio_display_widget.h"
#include "gluten/elements/image_button.h"
#include "gluten/elements/toolbar.h"
#include "gluten/theme/theme.h"
#include "gluten/utils/imgui_util_structures.h"
#include "imgui.h"
#include "managers/project_manager.h"

#include <rttr/type>

struct playable_selection
{
    sbk::core::object* selectedObject = nullptr;

    void set(sbk::core::object* object)
    {
        if (selectedObject != nullptr && object != selectedObject)
        {
            sbk_system_stop_all(0);
        }

        selectedObject = object;
    }

    auto get_name() -> std::string const
    {
        if (selectedObject != nullptr)
        {
            return std::string(selectedObject->try_convert_object<sbk::core::database_object>()->get_object_name());
        }
        return "No Selection";
    }
};

static playable_selection s_lastPlayableSelection;

void player_widget::start_implementation()
{
    widget::start_implementation();

    add_child_widget<audio_display_widget>(true)->set_visible_in_toolbar(true, false);
}

void player_widget::render_implementation()
{
    const gluten::imgui::scoped_font font(get_app()->get_font(gluten::fonts::regular_lucide_icons));
    const gluten::imgui::scoped_style morePadding(ImGuiStyleVar_WindowPadding, gluten::theme::paddingVec);

    const selection& selection                   = get_app()->get_manager_by_class<project_manager>()->get_selection();
    const std::optional<rttr::type> selectedType = selection.selected_type();
    s_lastPlayableSelection.set(selection.get_selected());

    ImGui::Begin(fmt::format("{} - {}", s_lastPlayableSelection.get_name(), get_widget_name().data()).c_str());

    const bool isSelected = !!selection.get_selected();
    const bool isPlayable =
        isSelected && selectedType.has_value() && sbk::util::type_helper::is_type_playable(selectedType.value());

    ImGui::BeginDisabled(!isSelected || !isPlayable);

    if (ImGui::IsKeyReleased(ImGuiKey_Space))
    {
        toggle_play_selected();
    }

    gluten::toolbar toolbar;
    toolbar.set_element_content_scale(1.5f);
    toolbar.get_element_anchor().set_achor_from_preset(gluten::element::anchor_preset::stretch_full);
    toolbar.set_element_window_padding();
    toolbar.set_layout_spacing(6.0f);
    toolbar.render_window();

    gluten::toolbar_button playButton(ICON_LC_PLAY);
    gluten::toolbar_button stopButton(ICON_LC_SQUARE);
    gluten::toolbar_button pauseButton(ICON_LC_PAUSE);

    if (toolbar.render_layout_element_pixels(&playButton, 64.0f, 64.0f))
    {
        play_selected();
    }

    if (toolbar.render_layout_element_pixels(&stopButton, 64.0f, 64.0f))
    {
        stop_selected();
    }

    if (toolbar.render_layout_element_pixels(&pauseButton, 64.0f, 64.0f))
    {
        stop_selected();
    }
    toolbar.finish_layout();

    ImGui::EndDisabled();

    // sbk::engine::node* const nodeSelection =
    //     s_lastPlayableSelection.selectedObject ?
    //     s_lastPlayableSelection.selectedObject->try_convert_object<sbk::engine::node>() : nullptr;

    // sbk::engine::global_parameter_list parameterList;

    // if (nodeSelection)
    //{
    //     nodeSelection->gather_parameters(parameterList);
    // }

    // sbk::engine::game_object* const listenerGameObject = sbk::engine::system::get()->get_listener_game_object();

    // if (ImGui::BeginTabBar("Runtime Parameters"))
    //{
    //     if (ImGui::BeginTabItem("Switches"))
    //     {
    //         if (ImGui::BeginTable("Table", 2))
    //         {
    //             for (const sbk::core::database_ptr<sbk::engine::named_parameter>& intParameter :
    //                  parameterList.intParameters)
    //             {
    //                 if (intParameter.lookup() == false)
    //                 {
    //                     continue;
    //                 }

    //                sbk::core::database_ptr<sbk::engine::named_parameter_value> selectedIntParameterValue =
    //                    listenerGameObject->get_int_parameter_value(intParameter);

    //                if (selectedIntParameterValue.lookup() == false)
    //                {
    //                    continue;
    //                }

    //                ImGui::TableNextColumn();
    //                ImGui::TextUnformatted(intParameter->get_database_name().data());

    //                ImGui::TableNextColumn();

    //                if (ImGui::BeginCombo("Selected", selectedIntParameterValue->get_database_name().data()))
    //                {
    //                    for (sbk::core::database_ptr<sbk::engine::named_parameter_value> parameterValue :
    //                         intParameter->get_values())
    //                    {
    //                        if (parameterValue.lookup() == false)
    //                        {
    //                            continue;
    //                        }

    //                        bool selected = parameterValue == selectedIntParameterValue;
    //                        if (ImGui::Selectable(parameterValue->get_database_name().data(), &selected))
    //                        {
    //                            listenerGameObject->set_int_parameter_value(
    //                                {intParameter.id(), parameterValue->get_database_id()});
    //                        }
    //                    }

    //                    ImGui::EndCombo();
    //                }
    //            }

    //            ImGui::EndTable();
    //        }

    //        ImGui::EndTabItem();
    //    }

    //    ImGui::EndTabBar();
    //}

    // render_children();

    ImGui::SetWindowFontScale(1.0f);

    ImGui::End();
}

void player_widget::play_selected()
{
    if (s_lastPlayableSelection.selectedObject)
    {
        if (sbk::engine::container* container = s_lastPlayableSelection.selectedObject->try_convert_object<sbk::engine::container>())
        {
            (void)sbk::engine::post_container(container->get_database_id(), 0);
        }
        else if (sbk::engine::sound* sound = s_lastPlayableSelection.selectedObject->try_convert_object<sbk::engine::sound>())
        {
            if (sbk::engine::sound_container* previewContainer = sbk::engine::system::get()->get_project()->get_preview_container().lock().get())
            {
                previewContainer->set_sound(sound);

                (void)sbk::engine::post_container(previewContainer->get_database_id(), 0);
            }
        }
        else if (sbk::engine::event* event = s_lastPlayableSelection.selectedObject->try_convert_object<sbk::engine::event>())
        {
            sbk_system_post_event(event->get_database_id(), 0);
        }
    }
}

void player_widget::stop_selected() { sbk_system_stop_all(0); }

void player_widget::toggle_play_selected()
{
    if (sbk::engine::system::get()->get_runtime()->get_listener_game_object()->is_playing())
    {
        stop_selected();
    }
    else
    {
        play_selected();
    }
}