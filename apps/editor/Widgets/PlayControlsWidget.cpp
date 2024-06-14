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
#include "sound_bakery/voice/node_instance.h"
#include "sound_bakery/voice/voice.h"

#include <rttr/type>

static SB::Core::Object* s_lastPlayableSelection;

void PlayerWidget::Start()
{
    Widget::Start();

    AddChildWidget<AudioDisplayWidget>();
}

void PlayerWidget::Render()
{
    ImGui::Begin("Player");

    Selection& selection    = GetApp()->GetProjectManager()->GetSelection();
    rttr::type selectedType = selection.SelectedType();

    const bool isSelected = !!selection.GetSelected();
    const bool isPlayable =
        isSelected && (selectedType.is_derived_from<SB::Engine::Container>() ||
                       selectedType.is_derived_from<SB::Engine::Sound>() ||
                       selectedType.is_derived_from<SB::Engine::Event>());

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
                    GetApp()->GetProjectManager()->GetPreviewSoundContainer())
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

    ImGui::EndDisabled();

    if (SB::Engine::GameObject* listener =
            SB::Engine::System::get()->getListenerGameObject())
    {
        if (std::size_t voicesSize = listener->voiceCount())
        {
            ImGui::Text("Number of playing voices {%lu}", voicesSize);

            for (std::size_t index = 0; index < voicesSize; ++index)
            {
                SB::Engine::Voice* voice = listener->getVoice(index);

                if (voice == nullptr)
                {
                    continue;
                }

                for (std::size_t instanceIndex = 0; instanceIndex < voice->voices();
                     ++instanceIndex)
                {
                    SB::Engine::NodeInstance* nodeInstance =
                        voice->voice(instanceIndex);

                    if (nodeInstance)
                    {
                        bool playing = nodeInstance->isPlaying();
                        ImGui::Text("Channel is %s",
                                    playing ? "playing" : "not playing");
                    }
                }
            }
        }
    }

    RenderChildren();

    ImGui::End();
}
