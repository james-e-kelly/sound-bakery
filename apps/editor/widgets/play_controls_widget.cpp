#include "play_controls_widget.h"

#include "app/app.h"
#include "audio_display_widget.h"
#include "managers/project_manager.h"
#include "imgui.h"
#include "sound_bakery/event/event.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/system.h"
#include "sound_bakery/util/type_helper.h"
#include "sound_bakery/voice/node_instance.h"
#include "sound_bakery/voice/voice.h"
#include "sound_bakery/editor/project/project.h"

#include <rttr/type>

static sbk::core::object* s_lastPlayableSelection;
//
//void PlayerWidget::start()
//{
//    widget::start();
//
//    add_child_widget<AudioDisplayWidget>();
//}
//
//void PlayerWidget::render()
//{
//    ImGui::Begin("Player");
//
//    Selection& selection    = get_app()->get_manager_by_class<ProjectManager>()->GetSelection();
//    std::optional<rttr::type> selectedType = selection.SelectedType();
//
//    const bool isSelected = !!selection.GetSelected();
//    const bool isPlayable =
//        isSelected && selectedType.has_value() && sbk::Util::TypeHelper::isTypePlayable(selectedType.value());
//
//    if (isPlayable)
//    {
//        // Stop playing previous if we're selecting something new that is playable
//        if (selection.GetSelected() != s_lastPlayableSelection)
//        {
//            //sbk::engine::system::get()->get_project()->listender->stopAll();
//        }
//        
//        s_lastPlayableSelection = selection.GetSelected();
//    }
//    else if (!isSelected)
//    {
//        s_lastPlayableSelection = nullptr;
//    }
//
//    ImGui::BeginDisabled(!isSelected);
//
//    if (isSelected && !!s_lastPlayableSelection)
//    {
//        ImGui::Text("%s", s_lastPlayableSelection
//                              ->try_convert_object<sbk::core::database_object>()
//                              ->get_database_name()
//                              .data());
//    }
//
//    if (ImGui::Button("Play"))
//    {
//        if (sbk::engine::Container* container =
//                s_lastPlayableSelection
//                    ->try_convert_object<sbk::engine::Container>())
//        {
//            sbk::engine::system::get()->getListenerGameObject()->playContainer(
//                container);
//        }
//        else if (sbk::engine::Sound* sound =
//                     s_lastPlayableSelection
//                         ->try_convert_object<sbk::engine::Sound>())
//        {
//            if (sbk::engine::SoundContainer* previewContainer =
//                    get_app()->GetProjectManager()->GetPreviewSoundContainer())
//            {
//                previewContainer->setSound(sound);
//
//                sbk::engine::system::get()
//                    ->getListenerGameObject()
//                    ->playContainer(previewContainer);
//            }
//        }
//        else if (sbk::engine::Event* event =
//                     s_lastPlayableSelection
//                         ->try_convert_object<sbk::engine::Event>())
//        {
//            sbk::engine::system::get()->getListenerGameObject()->postEvent(
//                event);
//        }
//    }
//
//    ImGui::SameLine();
//
//    if (ImGui::Button("Stop"))
//    {
//        sbk::engine::system::get()->getListenerGameObject()->stopAll();
//    }
//
//    sbk::engine::Node* const nodeSelection =
//        s_lastPlayableSelection ? s_lastPlayableSelection->try_convert_object<sbk::engine::Node>() : nullptr;
//
//    sbk::engine::GlobalParameterList parameterList;
//
//    if (nodeSelection)
//    {
//        nodeSelection->gatherParameters(parameterList);
//    }
//
//    sbk::engine::GameObject* const listenerGameObject = sbk::engine::system::get()->getListenerGameObject();
//
//    if (ImGui::BeginTabBar("Runtime Parameters"))
//    {
//        if (ImGui::BeginTabItem("Switches"))
//        {
//            if (ImGui::BeginTable("Table", 2))
//            {
//                for (const sbk::core::database_ptr<sbk::engine::NamedParameter>& intParameter :
//                     parameterList.intParameters)
//                {
//                    if (intParameter.lookup() == false)
//                    {
//                        continue;
//                    }
//
//                    sbk::core::database_ptr<sbk::engine::NamedParameterValue> selectedIntParameterValue =
//                        listenerGameObject->getIntParameterValue(intParameter);
//
//                    if (selectedIntParameterValue.lookup() == false)
//                    {
//                        continue;
//                    }
//
//                    ImGui::TableNextColumn();
//                    ImGui::TextUnformatted(intParameter->get_database_name().data());
//
//                    ImGui::TableNextColumn();
//
//                    if (ImGui::BeginCombo("Selected", selectedIntParameterValue->get_database_name().data()))
//                    {
//                        for (sbk::core::database_ptr<sbk::engine::NamedParameterValue> parameterValue : intParameter->getValues())
//                        {
//                            if (parameterValue.lookup() == false)
//                            {
//                                continue;
//                            }
//
//                            bool selected = parameterValue == selectedIntParameterValue;
//                            if (ImGui::Selectable(parameterValue->get_database_name().data(), &selected))
//                            {
//                                listenerGameObject->setIntParameterValue(
//                                    {intParameter.id(), parameterValue->get_database_id()});
//                            }
//                        }
//
//                        ImGui::EndCombo();
//                    }
//                }
//
//                ImGui::EndTable();
//            }
//            
//            
//            ImGui::EndTabItem();
//        }
//
//        ImGui::EndTabBar();
//    }
//
//    ImGui::EndDisabled();
//
//    render_children();
//
//    ImGui::End();
//}
