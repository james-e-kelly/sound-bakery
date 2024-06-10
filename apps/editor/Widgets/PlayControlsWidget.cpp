#include "PlayControlsWidget.h"

#include "App/App.h"
#include "AudioDisplayWidget.h"
#include "Managers/ProjectManager.h"
#include "imgui.h"
#include "sound_bakery/event/event.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/system.h"
#include "sound_bakery/util/type_helper.h"
#include "sound_bakery/voice/node_instance.h"
#include "sound_bakery/voice/voice.h"

#include <rttr/type>

static SB::Core::Object* s_lastPlayableSelection;

void PlayerWidget::start()
{
    widget::start();

    add_child_widget<AudioDisplayWidget>();
}

void PlayerWidget::Render()
{
    ImGui::Begin("Player");

    Selection& selection    = get_app()->GetProjectManager()->GetSelection();
    std::optional<rttr::type> selectedType = selection.SelectedType();

    const bool isSelected = !!selection.GetSelected();
    const bool isPlayable =
        isSelected && selectedType.has_value() && SB::Util::TypeHelper::isTypePlayable(selectedType.value());

    if (isPlayable)
    {
        // Stop playing previous if we're selecting something new that is playable
        if (selection.GetSelected() != s_lastPlayableSelection)
        {
            SB::Engine::System::get()->getListenerGameObject()->stopAll();
        }
        
        s_lastPlayableSelection = selection.GetSelected();
    }
    else if (!isSelected)
    {
        s_lastPlayableSelection = nullptr;
    }

    ImGui::BeginDisabled(!isSelected);

    if (isSelected && !!s_lastPlayableSelection)
    {
        ImGui::Text("%s", s_lastPlayableSelection
                              ->tryConvertObject<SB::Core::DatabaseObject>()
                              ->getDatabaseName()
                              .data());
    }

    if (ImGui::Button("Play"))
    {
        if (SB::Engine::Container* container =
                s_lastPlayableSelection
                    ->tryConvertObject<SB::Engine::Container>())
        {
            SB::Engine::System::get()->getListenerGameObject()->playContainer(
                container);
        }
        else if (SB::Engine::Sound* sound =
                     s_lastPlayableSelection
                         ->tryConvertObject<SB::Engine::Sound>())
        {
            if (SB::Engine::SoundContainer* previewContainer =
                    get_app()->GetProjectManager()->GetPreviewSoundContainer())
            {
                previewContainer->setSound(sound);

                SB::Engine::System::get()
                    ->getListenerGameObject()
                    ->playContainer(previewContainer);
            }
        }
        else if (SB::Engine::Event* event =
                     s_lastPlayableSelection
                         ->tryConvertObject<SB::Engine::Event>())
        {
            SB::Engine::System::get()->getListenerGameObject()->postEvent(
                event);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Stop"))
    {
        SB::Engine::System::get()->getListenerGameObject()->stopAll();
    }

    SB::Engine::Node* const nodeSelection =
        s_lastPlayableSelection ? s_lastPlayableSelection->tryConvertObject<SB::Engine::Node>() : nullptr;

    SB::Engine::GlobalParameterList parameterList;

    if (nodeSelection)
    {
        nodeSelection->gatherParameters(parameterList);
    }

    SB::Engine::GameObject* const listenerGameObject = SB::Engine::System::get()->getListenerGameObject();

    if (ImGui::BeginTabBar("Runtime Parameters"))
    {
        if (ImGui::BeginTabItem("Switches"))
        {
            if (ImGui::BeginTable("Table", 2))
            {
                for (const SB::Core::DatabasePtr<SB::Engine::NamedParameter>& intParameter :
                     parameterList.intParameters)
                {
                    if (intParameter.lookup() == false)
                    {
                        continue;
                    }

                    SB::Core::DatabasePtr<SB::Engine::NamedParameterValue> selectedIntParameterValue =
                        listenerGameObject->getIntParameterValue(intParameter);

                    if (selectedIntParameterValue.lookup() == false)
                    {
                        continue;
                    }

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(intParameter->getDatabaseName().data());

                    ImGui::TableNextColumn();

                    if (ImGui::BeginCombo("Selected", selectedIntParameterValue->getDatabaseName().data()))
                    {
                        for (SB::Core::DatabasePtr<SB::Engine::NamedParameterValue> parameterValue : intParameter->getValues())
                        {
                            if (parameterValue.lookup() == false)
                            {
                                continue;
                            }

                            bool selected = parameterValue == selectedIntParameterValue;
                            if (ImGui::Selectable(parameterValue->getDatabaseName().data(), &selected))
                            {
                                listenerGameObject->setIntParameterValue(
                                    {intParameter.id(), parameterValue->getDatabaseID()});
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

    ImGui::EndDisabled();

    render_children();

    ImGui::End();
}
