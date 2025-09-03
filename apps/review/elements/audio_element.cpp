#include "audio_element.h"

#include "gluten/subsystems/audio_subsystem.h"

audio_element::audio_element(const std::filesystem::path& filePath,
                                    const int64_t fileId)
    : file_element(gluten::anchor_preset::stretch_full, filePath, fileId)
{
    m_audioBackground.set_element_background_color(gluten::theme::carbon_g100::layer02);
}

auto audio_element::render_element(const ImRect& elementRect) -> bool
{
    file_element::render_element(elementRect);

    m_layout.render(elementRect);
    m_layout.render_layout_element_pixels_vertical(&m_audioBackground, elementRect.GetHeight() - s_controlHeight);
    m_layout.render_layout_element_pixels_vertical(&m_controlButtonsLayout, s_controlHeight);

    render_controls();

    render_waveform();

    if (ImGui::IsMouseHoveringRect(m_audioBackground.get_element_rect().Min, m_audioBackground.get_element_rect().Max))
    {
        const bool clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        const bool doubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

        if (clicked || doubleClicked)
        {
            const ImVec2 mousePos = ImGui::GetMousePos();
            const float mousePercentageInWaveform = (mousePos.x - m_audioBackground.get_element_rect().Min.x) / (m_audioBackground.get_element_rect().GetWidth());
            const float timeInWaveform = mousePercentageInWaveform * m_fileDuration;

            if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
            {
                audioSubsystem->set_sound_cursor_position(m_filePath, timeInWaveform);

                if (doubleClicked)
                {
                    audioSubsystem->play_sound(m_filePath);
                }
            }
        }
    }

    return false;
}

void audio_element::render_waveform()
{
    if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
    {
        if (ImDrawList* const drawList = ImGui::GetWindowDrawList())
        {
            typename gluten::audio_subsystem::waveform& waveform =
                audioSubsystem->get_ui_waveform(
                    m_filePath,
                    m_audioBackground.get_element_rect().GetWidth());

            const std::size_t buckets = waveform.size();

            if (buckets > 0)
            {
                const std::size_t channels = waveform[0].size();

                const float widthAvailable = m_audioBackground.get_element_rect().GetWidth();
                const float bucketWidth = 1.0f;

                const float heightAvailable = m_audioBackground.get_element_rect().GetHeight();
                const float heightToEachChannel = heightAvailable / channels;
                const float channelHalfHeight   = heightToEachChannel / 2.0f;

                for (std::size_t pixel = 0; pixel < widthAvailable; ++pixel)
                {
                    const float bucketStartX = m_audioBackground.get_element_rect().Min.x + pixel;

                    for (std::size_t channel = 0; channel < channels; ++channel)
                    {
                        const float channelStartY = m_audioBackground.get_element_rect().Min.y + (heightToEachChannel * channel);
                        const float channelMidY = channelStartY + channelHalfHeight;

                        if (waveform.size() > pixel)
                        {
                            const std::pair<float, float> minMax = waveform[pixel][channel];

                            ImVec2 minLine(bucketStartX, channelMidY - (minMax.first * channelHalfHeight));
                            ImVec2 maxLine(bucketStartX, channelMidY - (minMax.second * channelHalfHeight));

                            if (std::abs(maxLine.y - minLine.y) <= 1.0f)
                            {
                                minLine.y = channelMidY + 0.5f;
                                maxLine.y = channelMidY - 0.5f;
                            }

                            drawList->AddLine(minLine, maxLine, IM_COL32_WHITE);
                        }
                    }
                }
            }

            const float cursorX =
                m_audioBackground.get_element_rect().Min.x +
                (m_audioBackground.get_element_rect().GetWidth() *
                 m_filePercent);
            const ImVec2 cursorTop(cursorX,
                                   m_audioBackground.get_element_rect().Min.y);
            const ImVec2 cursorBottom(
                cursorX, m_audioBackground.get_element_rect().Max.y);

            drawList->AddLine(cursorBottom, cursorTop, IM_COL32_WHITE);
        }
    }
}

auto audio_element::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
{
    return ImVec2(parentSize.x, get_audio_height(parentSize.x) + s_controlHeight);
}

auto audio_element::get_file_play_position() const -> double
{
    if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
    {
        return audioSubsystem->get_sound_cursor_position(m_filePath);
    }
    return 0.0f;
}

auto audio_element::get_file_duration() const -> double
{
    if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
    {
        return audioSubsystem->get_sound_length(m_filePath);
    }
    return 0.0f;
}

auto audio_element::play_file() -> void
{
    if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
    {
        audioSubsystem->play_sound(m_filePath);
    }
}

auto audio_element::pause_file() -> void
{
    if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
    {
        audioSubsystem->pause_sound(m_filePath);
    }
}

auto audio_element::get_audio_height(float width) -> float
{
    return width * 0.33f;
}
