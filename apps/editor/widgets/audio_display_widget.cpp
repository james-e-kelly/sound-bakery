#include "audio_display_widget.h"

#include "app/app.h"
#include "managers/project_manager.h"
#include "imgui.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/system.h"
#include "sound_bakery/voice/node_instance.h"

int littleEndian24Bit(const void* bytes)
{
    return (ma_int32)((((ma_uint32) static_cast<const ma_uint8*>(bytes)[2]) << 16) |
                   (((ma_uint32) static_cast<const ma_uint8*>(bytes)[1]) << 8) |
                   ((ma_uint32) static_cast<const ma_uint8*>(bytes)[0]));
}
//
// float getAverageValueFromSampleRange(FMOD_SOUND* sound, unsigned int
// startIndex, unsigned int endIndex, unsigned int channelIndex)
//{
//    if (endIndex <= startIndex)
//    {
//        return 0;
//    }
//
//    FMOD_SOUND_FORMAT soundFormat;
//    int soundChannels;
//    int soundBits;
//
//    FMOD_Sound_GetFormat(sound, 0, &soundFormat, &soundChannels, &soundBits);
//
//    const int bytesPerSample = soundChannels * (soundBits / 8);
//    const int byteOffsetForSample = startIndex * bytesPerSample;
//    unsigned int samplesToCover = endIndex - startIndex;
//
//    void* samples = nullptr;
//    unsigned int samplesLength;
//
//    FMOD_RESULT lockResult = FMOD_Sound_Lock(sound, byteOffsetForSample,
//    samplesToCover * bytesPerSample, &samples, 0, &samplesLength, 0);
//
//    if (lockResult != FMOD_OK)
//    {
//        return 0;
//    }
//
//    if (samplesLength < samplesToCover * bytesPerSample)
//    {
//        return 0;
//    }
//
//    float sum = 0;
//    float maxValue = 1;
//
//    auto addSamples = [&]<typename T>(T* samplePointer, float inMaxValue)
//    {
//        while (samplesToCover)
//        {
//            sum += std::abs(*samplePointer++);
//            samplesToCover--;
//        }
//        maxValue = inMaxValue;
//    };
//
//    auto add24BitSamples = [&]<typename T>(T* samplePointer, float inMaxValue)
//    {
//        while (samplesToCover)
//        {
//            sum += std::abs(littleEndian24Bit(samplePointer));
//            samplePointer += 3;
//            samplesToCover--;
//        }
//        maxValue = inMaxValue;
//    };
//
//    switch(soundFormat)
//    {
//        case FMOD_SOUND_FORMAT_PCM8:
//            addSamples(static_cast<char*>(samples), CHAR_MAX);
//            break;
//        case FMOD_SOUND_FORMAT_PCM16:
//            addSamples(static_cast<short*>(samples), SHRT_MAX);
//            break;
//        case FMOD_SOUND_FORMAT_PCM24:
//            add24BitSamples(static_cast<char*>(samples), 8388607);
//            break;
//        case FMOD_SOUND_FORMAT_PCM32:
//            addSamples(static_cast<int*>(samples), INT_MAX);
//            break;
//
//        case FMOD_SOUND_FORMAT_PCMFLOAT:
//            addSamples(static_cast<float*>(samples), 1);
//            break;
//        case FMOD_SOUND_FORMAT_BITSTREAM:
//        case FMOD_SOUND_FORMAT_NONE:
//        case FMOD_SOUND_FORMAT_MAX:
//        case FMOD_SOUND_FORMAT_FORCEINT:
//            break;
//    }
//
//    FMOD_Sound_Unlock(sound, samples, 0, samplesLength, 0);
//
//    float result = (sum * 2.0f) / (float)(endIndex - startIndex);
//
//    return result / maxValue;
//}

void audio_display_widget::render()
{
    if (project_manager* manager = get_app()->get_manager_by_class<project_manager>())
    {
        if (sbk::core::object* selected = manager->get_selection().get_selected())
        {
            if (selected->getType() == sbk::engine::sound::type())
            {
                static float scale = 1.0f;
                ImGui::SliderFloat("Scale", &scale, 0.01f, 2.0f);

                if (ImGui::BeginChild("Waveform", ImVec2(0, 0), true))
                {
                    m_drawWidth = ImGui::GetWindowWidth();

                    const bool hasResized =
                        static_cast<int>(m_drawWidth) !=
                        static_cast<int>(m_previousDrawWidth);

                    m_previousDrawWidth = m_drawWidth;

                    const bool isPlaying = true;

                    if (isPlaying)
                    {
                        sbk::engine::SoundContainer* previewSound =
                            get_app()
                                ->get_manager_by_class<project_manager>()
                                ->get_preview_sound_container();
                        /*FMOD_SOUND* sound = previewSound ?
                        previewSound->getFSound() : nullptr;

                        unsigned int currentPostion = 0;
                        unsigned int soundLength = 0;

                        if (SB::Engine::game_object* listener =
                        SB::Engine::System::get()->get()->get_listener_game_object())
                        {
                            if (SB::Engine::Voice* voice =
                        listener->voiceCount() ? listener->getVoice(0) :
                        nullptr)
                            {
                                if (SB::Engine::NodeInstance* nodeInstance =
                        voice->voices() ? voice->voice(0) : nullptr)
                                {
                                    if (FMOD_CHANNEL* channel =
                        nodeInstance->getChannel())
                                    {
                                        FMOD_Channel_GetPosition(channel,
                        &currentPostion, FMOD_TIMEUNIT_PCM);
                                    }
                                }
                            }
                        }

                        FMOD_Sound_GetLength(sound, &soundLength,
                        FMOD_TIMEUNIT_PCM);

                        m_playPixel = (int)(m_drawWidth * ((float)currentPostion
                        / (float)soundLength));*/
                    }

                    if (has_cache() && !hasResized)
                    {
                        ImDrawList* const drawList = ImGui::GetWindowDrawList();

                        const ImVec2 cursorPos = ImGui::GetCursorScreenPos();

                        const float windowWidth  = ImGui::GetWindowWidth();
                        const float windowHeight = ImGui::GetWindowHeight();

                        const float windowPosX = cursorPos.x;
                        const float windowPosY = cursorPos.y;

                        for (int channel = 0; channel < m_cachedSamples.size();
                             channel++)
                        {
                            const float channelLaneHeight =
                                windowHeight / m_cachedSamples.size();
                            const float channelLaneHalfHeight =
                                channelLaneHeight / 2;
                            const float channelStartHeight =
                                windowPosY +
                                (channel + (channelLaneHeight * channel) +
                                 channelLaneHalfHeight);

                            for (int sample = 0;
                                 sample < m_cachedSamples[channel].size();
                                 sample++)
                            {
                                float sampleValue =
                                    m_cachedSamples[channel][sample] *
                                    channelLaneHeight * 0.5f * scale;

                                ImVec2 bottomPoint{
                                    cursorPos.x + static_cast<float>(sample),
                                    channelStartHeight + -sampleValue};
                                ImVec2 topPoint{
                                    cursorPos.x + static_cast<float>(sample),
                                    channelStartHeight + sampleValue};

                                drawList->AddLine(bottomPoint, topPoint,
                                                  IM_COL32_WHITE);
                            }
                        }

                        ImVec2 bottomPlayHead{cursorPos.x + m_playPixel,
                                              cursorPos.y + windowHeight};
                        ImVec2 topPlayHead{cursorPos.x + m_playPixel,
                                           cursorPos.y};

                        drawList->AddLine(bottomPlayHead, topPlayHead,
                                          IM_COL32(255, 0, 0, 255));
                    }
                    else
                    {
                        generate_cache();
                    }
                }

                ImGui::EndChild();
            }
            else
            {
                m_cachedSamples.clear();
                m_playPixel = 0;
            }
        }
    }
}

bool audio_display_widget::has_cache()
{
    sbk::engine::SoundContainer* currentNode =
        get_app()->get_manager_by_class<project_manager>()->get_preview_sound_container();

    if (currentNode)
    {
        sbk::engine::sound* currentSound = currentNode->getSound();

        const bool selectedNewSound = currentSound != m_previousSound;

        m_previousSound = currentSound;

        return !m_cachedSamples.empty() && !selectedNewSound;
    }
    return false;
}

void audio_display_widget::generate_cache()
{
    m_cachedSamples.clear();

    // SB::Engine::SoundContainer* preview =
    // GetApp()->GetProjectManager()->GetPreviewSoundContainer(); FMOD_SOUND*
    // sound = preview ? preview->getFSound() : nullptr;

    // if (sound)
    //{
    //     int soundChannels;
    //     unsigned int soundLength;

    //    FMOD_Sound_GetFormat(sound, 0, 0, &soundChannels, 0);
    //    FMOD_Sound_GetLength(sound, &soundLength, FMOD_TIMEUNIT_PCM);

    //    const int width = static_cast<int>(ImGui::GetWindowWidth());

    //    auto getSampleIndexFromPixel = [](int pixelIndex, int widgetWidth, int
    //    samplesInSound)
    //    {
    //        return (int)((float)samplesInSound * ((float)pixelIndex /
    //        (float)(widgetWidth)));
    //    };

    //    for (int channel = 0; channel < soundChannels; channel++)
    //    {
    //        std::vector<float> channelVector;
    //        for (int pixel = 0; pixel < width - 1; pixel++)
    //        {
    //            const int sampleOneIndex = getSampleIndexFromPixel(pixel,
    //            width, soundLength); const int sampleTwoIndex =
    //            getSampleIndexFromPixel(pixel + 1, width, soundLength);

    //            const float average = getAverageValueFromSampleRange(sound,
    //            sampleOneIndex, sampleTwoIndex, channel);

    //            channelVector.push_back(average);
    //        }
    //        m_cachedSamples.push_back(channelVector);
    //    }
    //}
}
