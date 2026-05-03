#include "audio_element.h"

#include "gluten/subsystems/audio_subsystem.h"
#include "managers/workspace_manager.h"
#include "implot.h"

namespace
{
    constexpr float g_commentBubbleRadius = 10.0f;
    constexpr float g_commentBubbleOffsetY = g_commentBubbleRadius + 4.0f;
}

audio_element::audio_element(const std::filesystem::path& filePath,
                                    const int64_t fileId)
    : file_element(gluten::anchor_preset::stretch_full, filePath, fileId)
{
    m_loudnessBackground.set_element_background_color(gluten::theme::layer02);

    m_layout.set_layout_spacing(gluten::theme::padding);
    m_layout.set_element_padding(gluten::theme::paddingVec);

    m_waveformAndLoudnessLayout.set_layout_spacing(gluten::theme::padding);
    m_waveformAndLoudnessLayout.set_element_rounding(gluten::theme::rounding);

    m_controlButtonsLayout.set_element_rounding(gluten::theme::rounding);
    m_controlButtonsLayout.set_element_background_color(gluten::theme::layer02);
    m_controlButtonsLayout.set_element_max_size(ImVec2(0.0f, s_controlButtonsWidth));

    m_audioBackground.set_element_background_color(gluten::theme::layer02);

    m_audioBackground.set_element_rounding(gluten::theme::rounding);
    m_loudnessBackground.set_element_rounding(gluten::theme::rounding);
}

auto audio_element::render_element(const gluten::element_render_info& renderInfo) -> bool
{
    file_element::render_element(renderInfo);

    m_layout.render(renderInfo.elementBox);
    m_layout.render_layout_element_pixels_vertical(&m_waveformAndLoudnessLayout, m_layout.get_element_rect().GetHeight() - s_controlHeight);
    m_layout.render_layout_element_pixels_vertical(&m_controlButtonsLayout, s_controlHeight);

    m_waveformAndLoudnessLayout.render_layout_element_percent_horizontal(&m_audioBackground, 1.0f);
    //m_waveformAndLoudnessLayout.render_layout_element_remaining(&m_loudnessBackground);

    const bool createdComment = render_controls();

    if (renderInfo.isVisible)
    {
        render_waveform();
        if (!render_comments())
        {
            handle_mouse_control();
            handle_keyboard_controls(m_audioBackground.get_element_rect());
        }
    }

    return createdComment;
}

static inline double transform_linear_to_decibel(double v, void*)
{
    return ma_volume_linear_to_db(std::clamp(v, -1.0, 1.0));
}

static inline double transform_decibel_to_linear(double v, void*)
{
    return ma_volume_db_to_linear(std::clamp(v, -96.0, 0.0));
}

int linear_volume_formatter(double value, char* buff, int size, void* data)
{
    while (value < -1.0)
    {
        value += 2.0;
    }

    return snprintf(buff, size, "%g", value);
}

int time_formatter(double value, char* buff, int size, void* data)
{
    const double minute = std::floor(value / 60.0);
    const double second = std::floor(std::fmod(value, 60.0));
    const double millisecond = std::fmod(value, 1.0);

    const bool isWholeSecond = millisecond < 0.1;
    
    if (millisecond > 0.0)
    {
        return snprintf(buff, size, "%02g:%02g.%2g", minute, second, std::floor(millisecond * 1000.0));
    }
    else
    {
        return snprintf(buff, size, "%02g:%02g", minute, second);
    }
}

auto audio_element::render_waveform() -> void
{
    if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
    {
        if (ImDrawList* const drawList = ImGui::GetWindowDrawList())
        {
            m_plotLimitLeft = ImGui::GetStateStorage()->GetFloat(ImGui::GetID("Min"));
            m_plotLimitRight = ImGui::GetStateStorage()->GetFloat(ImGui::GetID("Max"));

            gluten::audio_subsystem::waveform_lods& waveformLods = audioSubsystem->get_ui_waveform_lods(m_filePath, m_fileDuration);
            const auto& lufs = audioSubsystem->get_loudness_lufs(m_filePath);

            const float plotTimeWidth = m_plotLimitRight - m_plotLimitLeft;

            bool sampleRes = false;

            const gluten::audio_subsystem::waveform_lod_cache_type::cached_data* waveformCache = nullptr;
            const gluten::audio_subsystem::waveform_lod_cache_type::cached_data* peakCache = nullptr;

            if (plotTimeWidth < 0.05f)
            {
                waveformCache = &waveformLods.sampleRes.get_cached_data(m_filePath);
                peakCache     = &waveformLods.highRes.get_cached_data(m_filePath);
                sampleRes     = true;
            }
            else if (plotTimeWidth < 1.0f)
            {
                waveformCache = &waveformLods.highRes.get_cached_data(m_filePath);
                peakCache     = &waveformLods.highRes.get_cached_data(m_filePath);
            }
            else if (plotTimeWidth < 10.0f)
            {
                waveformCache = &waveformLods.medRes.get_cached_data(m_filePath);
                peakCache     = &waveformLods.medRes.get_cached_data(m_filePath);
            }
            else if (plotTimeWidth < 30.0f)
            {
                waveformCache = &waveformLods.lowRes.get_cached_data(m_filePath);
                peakCache     = &waveformLods.lowRes.get_cached_data(m_filePath);
            }
            else
            {
                waveformCache = &waveformLods.thumbnailRes.get_cached_data(m_filePath);
                peakCache     = &waveformLods.thumbnailRes.get_cached_data(m_filePath);
            }

            if (waveformCache && waveformCache->m_state != gluten::cache_state::has_data)
            {
                waveformCache = &waveformLods.thumbnailRes.get_cached_data(m_filePath);
            }

            if (peakCache && peakCache->m_state != gluten::cache_state::has_data)
            {
                peakCache = &waveformLods.thumbnailRes.get_cached_data(m_filePath);
            }

            if (!waveformCache || !peakCache || waveformCache->m_state != gluten::cache_state::has_data || peakCache->m_state != gluten::cache_state::has_data)
            {
                return;
            }

            const std::size_t channels = waveformCache->m_cache.waveform[0].size();

            ImGui::SetCursorScreenPos(m_audioBackground.get_element_rect().GetTL());

            float value = ImGui::GetStateStorage()->GetFloat(ImGui::GetID("Min dB"), -96.0f);
            ImGui::SliderFloat("Min dB", &value, -96.0f, 0.0f);

            if (ImPlot::BeginPlot(fmt::format("Waveform##{}", m_filePath.filename().string()).c_str(), ImVec2(-1, m_audioBackground.get_element_rect().GetHeight()), ImPlotFlags_NoTitle | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText))
            {
                ImPlot::SetupAxis(ImAxis_X1, "Time", ImPlotAxisFlags_None);
                ImPlot::SetupAxisFormat(ImAxis_X1, time_formatter);
                ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, m_fileDuration);
                ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, 0.0, m_fileDuration);

                const float minYValue = 1.0f - (2.0f * channels);
                const float mindBYValue = value * channels;

                static const char* volumeLabels[]    = {"+1.0", "+0.5", "0", "-0.5", "-1.0"};
                static const double volumeValueValues[] = {1.0, 0.5, 0.0, -0.5, -1.0};

                ImPlot::SetupAxis(ImAxis_Y1, "Volume", ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_Lock);
                ImPlot::SetupAxisLimits(ImAxis_Y1, minYValue, 1.0);
                ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, minYValue, 1.0);
                ImPlot::SetupAxisFormat(ImAxis_Y1, linear_volume_formatter);

                ImPlot::SetupAxis(ImAxis_Y2, "dB", ImPlotAxisFlags_AuxDefault | ImPlotAxisFlags_Lock);
                ImPlot::SetupAxisLimits(ImAxis_Y2, mindBYValue, 0.0, ImPlotCond_Always);
                ImPlot::SetupAxisLimitsConstraints(ImAxis_Y2, mindBYValue, 0.0);

                ImPlot::SetupAxis(ImAxis_Y3, "LU", ImPlotAxisFlags_AuxDefault | ImPlotAxisFlags_Lock);
                ImPlot::SetupAxisLimits(ImAxis_Y3, -48.0, 0.0);
                ImPlot::SetupAxisLimitsConstraints(ImAxis_Y3, -48.0, 0.0);

                for (int channel = 0; channel < channels; ++channel)
                {
                    std::vector<ImVec2> upperPoints;
                    std::vector<ImVec2> lowerPoints;
                    std::vector<ImVec2> dbPoints;
                    std::vector<ImVec2> rmsPoints;
                    std::vector<ImVec2> samplePoints;

                    const int numWaveformPoints  = (int)(m_fileDuration * waveformCache->m_cache.resolution);
                    const int plotLimitLeftAsPointIndexAtCurrentRes = (m_plotLimitLeft / m_fileDuration) * numWaveformPoints;  // Try to skip iterating points that won't be rendered
                    const int plotLimitRightAsPointIndexAtCurrentRes = (m_plotLimitRight / m_fileDuration) * numWaveformPoints;

                    for (int point = plotLimitLeftAsPointIndexAtCurrentRes; point < numWaveformPoints && point <= plotLimitRightAsPointIndexAtCurrentRes; ++point)
                    {
                        const double xPosition = (double)point / (numWaveformPoints) * m_fileDuration;
                        const auto channelData   = waveformCache->m_cache.waveform[point][channel];

                        if (xPosition >= m_plotLimitLeft && xPosition <= m_plotLimitRight)
                        {
                            if (sampleRes)
                            {
                                samplePoints.push_back(ImVec2(xPosition, channelData.averageSample - (2.0f * channel)));
                            }
                            else
                            {
                                upperPoints.push_back(ImVec2(xPosition, channelData.max - (2.0f * channel)));
                                lowerPoints.push_back(ImVec2(xPosition, channelData.min - (2.0f * channel)));
                            }

                        }
                    }

                    const int numPeakPoints = (int)(m_fileDuration * peakCache->m_cache.resolution);

                    const int plotLimitLeftAsPointIndexAtLowRes = (m_plotLimitLeft / m_fileDuration) * numPeakPoints;
                    const int plotLimitRighttAsPointIndexAtLowRes = (m_plotLimitRight / m_fileDuration) * numPeakPoints;

                    for (int point = plotLimitLeftAsPointIndexAtLowRes; point < numPeakPoints && point <= plotLimitRighttAsPointIndexAtLowRes; ++point)
                    {
                        const double xPosition = (double)point / (numPeakPoints) * m_fileDuration;

                        const auto& lowResChannelData = peakCache->m_cache.waveform[point][channel];
                        const float rmsDecibel       = ma_volume_linear_to_db(lowResChannelData.rms);

                        dbPoints.push_back(ImVec2(xPosition, std::max(ma_volume_linear_to_db(std::max(-lowResChannelData.min, lowResChannelData.max)), value + 1.0f) + (value * channel)));
                        rmsPoints.push_back(ImVec2(xPosition, std::max(rmsDecibel, value + 1.0f) + (value * channel)));
                    }

                    ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);

                    if (sampleRes)
                    {
                        ImPlot::PlotStairsG("Waveform", [](int idx, void* data) { return ImPlotPoint(((ImVec2*)data)[idx]); }, samplePoints.data(), samplePoints.size());
                    }
                    else
                    {
                        ImPlot::PlotShadedG("Waveform", [](int idx, void* data)
                                            { return ImPlotPoint(((ImVec2*)data)[idx]); }, upperPoints.data(), [](int idx, void* data)
                                            { return ImPlotPoint(((ImVec2*)data)[idx]); }, lowerPoints.data(), lowerPoints.size());
                        ImPlot::PlotLineG("Waveform", [](int idx, void* data)
                                            { return ImPlotPoint(((ImVec2*)data)[idx]); }, upperPoints.data(), upperPoints.size());
                        ImPlot::PlotLineG("Waveform", [](int idx, void* data)
                                            { return ImPlotPoint(((ImVec2*)data)[idx]); }, lowerPoints.data(), lowerPoints.size());
                    }

                    ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);

                    if (sampleRes)
                    {
                        ImPlot::PlotStairsG("Peak", [](int idx, void* data) { return ImPlotPoint(((ImVec2*)data)[idx]); }, dbPoints.data(), dbPoints.size());
                        ImPlot::PlotStairsG("RMS", [](int idx, void* data) { return ImPlotPoint(((ImVec2*)data)[idx]); }, rmsPoints.data(), rmsPoints.size());
                    }
                    else
                    {
                        ImPlot::PlotLineG("Peak", [](int idx, void* data) { return ImPlotPoint(((ImVec2*)data)[idx]); }, dbPoints.data(), dbPoints.size());
                        ImPlot::PlotLineG("RMS", [](int idx, void* data) { return ImPlotPoint(((ImVec2*)data)[idx]); }, rmsPoints.data(), rmsPoints.size());
                    }

                    ImPlot::SetAxes(ImAxis_X1, ImAxis_Y3);

                    ImPlot::TagY(lufs.m_cache.integrated, gluten::theme::supportInfo, fmt::format("LUFS-I: {:.1f}", lufs.m_cache.integrated).c_str());
                    ImPlot::TagY(lufs.m_cache.shorttermMax, gluten::theme::supportWarning, fmt::format("LUFS-S: {:.1f}", lufs.m_cache.shorttermMax).c_str());
                    ImPlot::TagY(lufs.m_cache.momentaryMax, gluten::theme::supportError, fmt::format("LUFS-M: {:.1f}", lufs.m_cache.momentaryMax).c_str());

                    ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
                    const double scaledFilePosition = m_filePosition;
                    ImPlot::PlotInfLines("##Playhead", &scaledFilePosition, 1);

                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImPlot::IsPlotHovered())
                    {
                        audioSubsystem->set_sound_cursor_position(m_filePath, ImPlot::GetPlotMousePos().x);
                    }

                    m_plotLimitLeft = ImPlot::GetPlotLimits().X.Min;
                    m_plotLimitRight = ImPlot::GetPlotLimits().X.Max;

                }
                ImPlot::EndPlot();
            }

            ImGui::GetStateStorage()->SetFloat(ImGui::GetID("Min"), m_plotLimitLeft);
            ImGui::GetStateStorage()->SetFloat(ImGui::GetID("Max"), m_plotLimitRight);
            ImGui::GetStateStorage()->SetFloat(ImGui::GetID("Min dB"), value);

            //if (audioSubsystem->get_sound_is_looping(m_filePath))
            //{
            //    const auto render_loop_line = [](float loopTime, float fileDuration, const ImRect& elementRect, ImDrawList* drawList) 
            //        {
            //            const float loopCursorPosition = elementRect.Min.x + (elementRect.GetWidth() * (loopTime / fileDuration));
            //            const ImVec2 loopCursorTop(loopCursorPosition, elementRect.Max.y);
            //            const ImVec2 loopCursorBottom(loopCursorPosition, elementRect.Min.y);

            //            drawList->AddLine(loopCursorTop, loopCursorBottom, IM_COL32(100,255,255,255));
            //        };

            //    const float loopStart = audioSubsystem->get_sound_loop_start_position(m_filePath);
            //    const float loopEnd = audioSubsystem->get_sound_loop_end_position(m_filePath);
            //    const float loopStartCursorPosition = m_audioBackground.get_element_rect().Min.x + (m_audioBackground.get_element_rect().GetWidth() * (loopStart / m_fileDuration));
            //    const float loopEndCursorPosition = m_audioBackground.get_element_rect().Min.x + (m_audioBackground.get_element_rect().GetWidth() * (loopEnd / m_fileDuration));

            //    render_loop_line(loopStart, m_fileDuration, m_audioBackground.get_element_rect(), drawList);
            //    render_loop_line(loopEnd, m_fileDuration, m_audioBackground.get_element_rect(), drawList);

            //    drawList->AddRectFilled(ImVec2(loopStartCursorPosition, m_audioBackground.get_element_rect().Min.y), ImVec2(loopEndCursorPosition, m_audioBackground.get_element_rect().Max.y), IM_COL32(100,100,100,100));
            //}
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
                                                              ImGui::ColorConvertFloat4ToU32(gluten::theme::supportWarning));
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
    return;

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
                audioSubsystem->set_sound_loop_start_position(m_filePath, timeInWaveform);
            }
            else if (dragging)
            {
                audioSubsystem->set_sound_loop_end_position(m_filePath, timeInWaveform);
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
