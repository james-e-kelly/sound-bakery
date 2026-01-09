#include "audio_element.h"

#include "gluten/subsystems/audio_subsystem.h"
#include "managers/workspace_manager.h"

namespace
{
    constexpr float g_commentBubbleRadius = 10.0f;
    constexpr float g_commentBubbleOffsetY = g_commentBubbleRadius + 4.0f;
}

audio_element::audio_element(const std::filesystem::path& filePath,
                                    const int64_t fileId)
    : file_element(gluten::anchor_preset::stretch_full, filePath, fileId)
{
    m_audioBackground.set_element_background_color(gluten::theme::carbon_g100::layerHover01);
    m_loudnessBackground.set_element_background_color(gluten::theme::gray100Hover);
}

auto audio_element::render_element(const ImRect& elementRect) -> bool
{
    file_element::render_element(elementRect);

    m_layout.render(elementRect);
    m_layout.render_layout_element_pixels_vertical(&m_waveformAndLoudnessLayout, elementRect.GetHeight() - s_controlHeight);

    m_waveformAndLoudnessLayout.render_layout_element_percent_horizontal(&m_audioBackground, 0.9f);
    m_waveformAndLoudnessLayout.render_layout_element_percent_horizontal(&m_loudnessBackground, 0.1f);

    m_layout.render_layout_element_pixels_vertical(&m_controlButtonsLayout, s_controlHeight);

    const bool createdComment = render_controls();

    render_waveform();
    if (!render_comments())
    {
        handle_mouse_control();
        handle_keyboard_controls(m_audioBackground.get_element_rect());
    }

    if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
    {
        auto lufs = audioSubsystem->get_loudness_lufs(m_filePath);

        if (lufs.has_data())
        {
            ImGui::SetCursorScreenPos(m_loudnessBackground.get_element_rect().GetTL());

            ImGui::BeginGroup();

            ImGui::NewLine();
            ImGui::Text("Integrated: %.1lf", lufs.m_cache.integrated);
            ImGui::Text("Maximum short-term: %.1lf", lufs.m_cache.shorttermMax);
            ImGui::Text("Maximum momentary: %.1lf", lufs.m_cache.momentaryMax);

            ImGui::EndGroup();
        }
    }

    return createdComment;
}

auto audio_element::render_waveform() -> void
{
    if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
    {
        auto lufs = audioSubsystem->get_loudness_lufs(m_filePath);

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

                            if (minMax.first < -1.0f || minMax.second > 1.0f)
                            {
                                drawList->AddLine(minLine, maxLine, ImGui::ColorConvertFloat4ToU32(gluten::theme::carbon_g100::textError));
                            }
                            else
                            {
                                drawList->AddLine(minLine, maxLine, ImGui::ColorConvertFloat4ToU32(gluten::theme::carbon_g100::textHelper));
                            }
                        }
                    }
                }
            }

            const float cursorX = m_audioBackground.get_element_rect().Min.x + (m_audioBackground.get_element_rect().GetWidth() * m_filePercent);
            const ImVec2 cursorTop(cursorX, m_audioBackground.get_element_rect().Min.y);
            const ImVec2 cursorBottom(cursorX, m_audioBackground.get_element_rect().Max.y);

            drawList->AddLine(cursorBottom, cursorTop, IM_COL32_WHITE);

            const float loopPosition = audioSubsystem->get_sound_loop_position(m_filePath);
            const float loopCursorPosition = m_audioBackground.get_element_rect().Min.x + (m_audioBackground.get_element_rect().GetWidth() * (loopPosition / m_fileDuration));
            const ImVec2 loopCursorTop(loopCursorPosition, m_audioBackground.get_element_rect().Min.y);
            const ImVec2 loopCursorBottom(loopCursorPosition, m_audioBackground.get_element_rect().Max.y);


            if (audioSubsystem->get_sound_is_looping(m_filePath))
            {
                if (cursorX < loopCursorPosition)
                {
                    drawList->AddRectFilled(cursorTop, loopCursorBottom, IM_COL32(100,100,100,100));
                }
                else
                {
                    drawList->AddRectFilled(cursorTop, loopCursorBottom, IM_COL32(255,100,100,100));
                }
                drawList->AddLine(loopCursorBottom, loopCursorTop, IM_COL32_WHITE);
            }
        }
    }
}

auto audio_element::render_comments() -> bool
{
    if (std::shared_ptr<workspace_manager> workspaceManager = gluten::app::get()->get_manager_by_class<workspace_manager>())
    {
        if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
        {
            if (ImDrawList* const drawList = ImGui::GetWindowDrawList())
            {
                const auto& comments = workspaceManager->get_all_comments_for_review(workspaceManager->get_selected_review().m_reviewId);

                if (comments.has_data())
                {
                    for (const auto& comment : comments.m_cache)
                    {
                        if (comment.m_fileId == m_fileId)
                        {
                            if (comment.m_timeStart >= 0.0) 
                            {
                                const float commentTimelineWidth = m_audioBackground.get_element_rect().GetWidth();
                                const float commentPosition = m_audioBackground.get_element_rect().Min.x + (commentTimelineWidth * (comment.m_timeStart / m_fileDuration));
                                const float commentBubbleCenterY = m_audioBackground.get_element_rect().Min.y + g_commentBubbleOffsetY;

                                if (std::abs(commentPosition) < 10000.0f)
                                {
                                    ImRect circleRect(commentPosition - g_commentBubbleRadius, commentBubbleCenterY - g_commentBubbleRadius, commentPosition + g_commentBubbleRadius, commentBubbleCenterY + g_commentBubbleRadius);

                                    drawList->AddCircleFilled(ImVec2(commentPosition, commentBubbleCenterY), g_commentBubbleRadius,
                                                              ImGui::ColorConvertFloat4ToU32(gluten::theme::purple50));
                                    ImGui::ItemAdd(circleRect, ImGui::GetID(comment.m_commentId));
                                    ImGui::SetItemTooltip(fmt::format("{}: {}", workspaceManager->get_user(comment.m_userId).m_displayName, comment.m_comment).c_str());

                                    if (ImGui::IsItemClicked())
                                    {
                                        audioSubsystem->set_sound_cursor_position(m_filePath, comment.m_timeStart);
                                        return true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}

auto audio_element::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
{
    return ImVec2(parentSize.x, get_audio_height(parentSize.x) + s_controlHeight);
}

auto audio_element::handle_mouse_control() -> void
{
    handle_mouse_controls(m_audioBackground.get_element_rect());

    if (ImGui::IsWindowFocused() && ImGui::IsMouseHoveringRect(m_audioBackground.get_element_rect().Min, m_audioBackground.get_element_rect().Max))
    {
        const bool clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        const bool dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
        
        const ImVec2 mousePos = ImGui::GetMousePos();
        const float mousePercentageInWaveform = (mousePos.x - m_audioBackground.get_element_rect().Min.x) / (m_audioBackground.get_element_rect().GetWidth());
        const float timeInWaveform = mousePercentageInWaveform * m_fileDuration;

        if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
        {
            if (clicked)
            {
                audioSubsystem->set_sound_cursor_position(m_filePath, timeInWaveform);
            }
            else if (dragging)
            {
                audioSubsystem->set_sound_loop_position(m_filePath, timeInWaveform);
            }
        }
    }
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
        audioSubsystem->pause_all();
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

auto audio_element::prev_frame() -> void
{
    if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
    {
        audioSubsystem->set_sound_cursor_position(m_filePath, std::max<float>(m_filePosition - 1.0f, 0.0f));
    }
}

auto audio_element::next_frame() -> void
{
    if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
    {
        audioSubsystem->set_sound_cursor_position(m_filePath, std::min<float>(m_filePosition + 1.0f, m_fileDuration));
    }
}

auto audio_element::get_is_playing() -> bool
{
    bool playing = false;

    if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
    {
        playing = audioSubsystem->get_sound_is_playing(m_filePath);
    }

    return playing;
}

auto audio_element::get_audio_height(float width) -> float
{
    return width * 0.33f;
}

auto audio_element::seek_to_position(double position) -> void
{
    if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
    {
        audioSubsystem->set_sound_cursor_position(m_filePath, position);
    }
}
