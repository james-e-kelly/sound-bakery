#include "video_element.h"

#include "gluten/app/app.h"
#include "gluten/elements/audio_element.h"
#include "gluten/subsystems/video_subsystem.h"
#include "gluten/theme/theme.h"
#include "imgui_internal.h"

namespace
{
    constexpr float s_defaultAspectRatio = 16.0f / 9.0f;
    constexpr float s_actionSafeScale    = 0.93f;
    constexpr float s_titleSafeScale     = 0.90f;
    constexpr double s_frameEpsilon      = 1e-6;  //< mpv reports frame n's time as n / fps, which can land a hair below it

    auto text_font() -> ImFont* { return gluten::app::get()->get_font(gluten::fonts::regular); }

    auto shrink_about_center(const ImRect& rect, float scale) -> ImRect
    {
        const ImVec2 center   = rect.GetCenter();
        const ImVec2 halfSize = ImVec2(rect.GetWidth() * scale * 0.5f, rect.GetHeight() * scale * 0.5f);
        return ImRect(ImVec2(std::floor(center.x - halfSize.x) + 0.5f, std::floor(center.y - halfSize.y) + 0.5f),
                      ImVec2(std::floor(center.x + halfSize.x) + 0.5f, std::floor(center.y + halfSize.y) + 0.5f));
    }

    auto frame_at(double seconds, double frameRate) -> long long
    {
        return static_cast<long long>(std::floor(std::max(seconds, 0.0) * frameRate + s_frameEpsilon));
    }

    auto format_frame_rate(double frameRate) -> std::string
    {
        std::string text = fmt::format("{:.3f}", frameRate);
        text.erase(text.find_last_not_of('0') + 1);
        if (!text.empty() && text.back() == '.')
        {
            text.pop_back();
        }
        return text;
    }

    auto format_codec(const std::string& codec) -> std::string
    {
        static const std::unordered_map<std::string, std::string> s_names = {
            {"h264", "H.264"}, {"hevc", "HEVC"}, {"av1", "AV1"}, {"vp8", "VP8"}, {"vp9", "VP9"}, {"prores", "ProRes"}, {"dnxhd", "DNxHD"}, {"mpeg4", "MPEG-4"}, {"mpeg2video", "MPEG-2"}, {"mjpeg", "MJPEG"}, {"aac", "AAC"}, {"mp3", "MP3"}, {"opus", "Opus"}, {"vorbis", "Vorbis"}, {"ac3", "AC-3"}, {"eac3", "E-AC-3"}, {"flac", "FLAC"}, {"alac", "ALAC"}, {"dts", "DTS"}, {"truehd", "TrueHD"}};

        if (const auto name = s_names.find(codec); name != s_names.end())
        {
            return name->second;
        }
        if (codec.starts_with("pcm_"))
        {
            return "PCM";
        }

        std::string upper = codec;
        std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char character)
                       { return static_cast<char>(std::toupper(character)); });
        return upper;
    }

    auto format_megabytes(int64_t bytes) -> std::string
    {
        return fmt::format("{:.1f}", static_cast<double>(bytes) / (1000.0 * 1000.0));
    }
}  // namespace

namespace gluten
{
    auto format_frame_timecode(double seconds, double frameRate) -> std::string
    {
        const long long nominalRate = std::max<long long>(std::llround(frameRate), 1);
        const long long frame       = frame_at(seconds, frameRate);

        const long long frames  = frame % nominalRate;
        const long long total   = frame / nominalRate;
        const long long hours   = total / 3600;
        const long long minutes = (total / 60) % 60;
        const long long secs    = total % 60;

        return fmt::format("{:02}:{:02}:{:02}:{:02}", hours, minutes, secs, frames);
    }

    video_element::video_element(const std::filesystem::path& videoFile)
        : file_element(anchor_preset::stretch_full, videoFile)
    {
        if (m_videoSubsystem.expired())
        {
            m_videoSubsystem = app::get()->get_subsystem_by_class<video_subsystem>();
        }

        if (std::shared_ptr<video_subsystem> videoSubsystem = m_videoSubsystem.lock())
        {
            if (videoSubsystem->get_video_texture(m_filePath) == 0)
            {
                videoSubsystem->load_video(m_filePath);
            }

            m_videoTexture = videoSubsystem->get_video_texture(m_filePath);
        }
    }

    auto video_element::get_info() const -> const video_info*
    {
        std::shared_ptr<video_subsystem> videoSubsystem = m_videoSubsystem.lock();
        return videoSubsystem ? videoSubsystem->get_video_info(m_filePath) : nullptr;
    }

    auto video_element::get_frame_rate() const -> double
    {
        const video_info* const info = get_info();
        return info ? info->frameRate : 0.0;
    }

    auto video_element::format_time(double seconds) const -> std::string
    {
        const double frameRate = get_frame_rate();
        if (frameRate <= 0.0)
        {
            return format_timecode(seconds);
        }

        return s_timeDisplay == video_time_display::frames ? fmt::format("{}", frame_at(seconds, frameRate)) : format_frame_timecode(seconds, frameRate);
    }

    auto video_element::get_aspect_ratio() const -> float
    {
        const video_info* const info = get_info();
        if (info && info->displayWidth > 0 && info->displayHeight > 0)
        {
            return static_cast<float>(info->displayWidth) / static_cast<float>(info->displayHeight);
        }
        return s_defaultAspectRatio;
    }

    auto video_element::get_viewport_height(float width) const -> float
    {
        return std::floor(std::clamp(width / get_aspect_ratio(), s_minViewportHeight, s_maxViewportHeight));
    }

    auto video_element::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
    {
        const float timelineHeight = s_rulerHeight + s_markerLaneHeight + s_scrubHeight;
        return ImVec2(parentSize.x, get_viewport_height(parentSize.x) + theme::space04 + timelineHeight + s_statusBarHeight);
    }

    auto video_element::render_element(const element_render_info& renderInfo) -> bool
    {
        ZoneScoped;

        std::shared_ptr<video_subsystem> videoSubsystem = m_videoSubsystem.lock();
        if (!videoSubsystem || !renderInfo.isVisible)
        {
            return false;
        }

        // The texture can be recreated when mpv drops and reloads a file, so don't hold on to a stale id.
        m_videoTexture = videoSubsystem->get_video_texture(m_filePath);

        const ImRect area          = renderInfo.elementBox;
        const float timelineHeight = s_rulerHeight + s_markerLaneHeight + s_scrubHeight;
        const float statusBarTop   = area.Max.y - s_statusBarHeight;
        const float timelineTop    = statusBarTop - timelineHeight;
        const float viewportBottom = std::max(area.Min.y, timelineTop - theme::space04);

        const ImRect viewport(area.Min, ImVec2(area.Max.x, viewportBottom));
        const ImRect timeline(ImVec2(area.Min.x, timelineTop), ImVec2(area.Max.x, statusBarTop));
        const ImRect statusBar(ImVec2(area.Min.x, statusBarTop), area.Max);

        render_viewport(viewport);
        render_timeline(timeline);
        render_status_bar(statusBar);

        handle_keyboard_controls(area);
        handle_mouse_controls(viewport);

        return false;
    }

    auto video_element::render_viewport(const ImRect& viewport) -> void
    {
        ImDrawList* const drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(viewport.Min, viewport.Max, ImGui::GetColorU32(theme::background), theme::radiusSm);

        const float aspect     = get_aspect_ratio();
        const float frameWidth = std::min(viewport.GetWidth(), viewport.GetHeight() * aspect);
        const ImVec2 frameSize = ImVec2(std::floor(frameWidth), std::floor(frameWidth / aspect));
        const ImVec2 frameMin  = ImVec2(std::floor(viewport.GetCenter().x - frameSize.x * 0.5f), std::floor(viewport.GetCenter().y - frameSize.y * 0.5f));
        const ImRect frame(frameMin, ImVec2(frameMin.x + frameSize.x, frameMin.y + frameSize.y));

        if (m_videoTexture != 0)
        {
            drawList->AddImage(static_cast<ImTextureID>(static_cast<intptr_t>(m_videoTexture)), frame.Min, frame.Max);
        }

        drawList->PushClipRect(frame.Min, frame.Max, true);

        if (s_showSafeAreas)
        {
            const ImRect actionSafe = shrink_about_center(frame, s_actionSafeScale);
            const ImRect titleSafe  = shrink_about_center(frame, s_titleSafeScale);
            drawList->AddRect(actionSafe.Min, actionSafe.Max, ImGui::GetColorU32(theme::adjust_alpha(theme::textPrimary, 0.45f)));
            drawList->AddRect(titleSafe.Min, titleSafe.Max, ImGui::GetColorU32(theme::adjust_alpha(theme::textPrimary, 0.25f)));
        }

        if (s_showTimecodeBurnIn)
        {
            const double frameRate     = get_frame_rate();
            const std::string timecode = frameRate > 0.0 ? format_frame_timecode(m_filePosition, frameRate) : format_timecode(m_filePosition);
            const float fontSize       = theme::textSizeBody;
            const float textWidth      = calc_tabular_text_width(text_font(), fontSize, timecode);
            const ImVec2 boxMax        = ImVec2(frame.Max.x - theme::space08, frame.Max.y - theme::space08);
            const ImVec2 boxMin        = ImVec2(boxMax.x - textWidth - theme::space08 * 2.0f, boxMax.y - fontSize - theme::space04 * 2.0f);

            drawList->AddRectFilled(boxMin, boxMax, ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.72f)), 2.0f);
            draw_tabular_text(drawList, text_font(), fontSize, ImVec2(boxMin.x + theme::space08, boxMin.y + theme::space04 - 1.0f), ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), timecode);
        }

        drawList->PopClipRect();

        if (s_showFrameMarkers)
        {
            render_frame_overlay(frame);
        }
    }

    auto video_element::render_timeline(const ImRect& timeline) -> void
    {
        ImDrawList* const drawList = ImGui::GetWindowDrawList();
        ImFont* const font         = text_font();

        const ImRect ruler(timeline.Min, ImVec2(timeline.Max.x, timeline.Min.y + s_rulerHeight));
        const ImRect markerLane(ImVec2(timeline.Min.x, ruler.Max.y), ImVec2(timeline.Max.x, ruler.Max.y + s_markerLaneHeight));
        const ImRect scrubRow(ImVec2(timeline.Min.x, markerLane.Max.y), timeline.Max);

        const ImU32 dividerColor = ImGui::GetColorU32(theme::background);
        drawList->AddRectFilled(ruler.Min, ruler.Max, ImGui::GetColorU32(theme::layer02), theme::radiusSm, ImDrawFlags_RoundCornersTop);
        drawList->AddRectFilled(markerLane.Min, scrubRow.Max, ImGui::GetColorU32(ImLerp(theme::background, theme::layer01, 0.5f)));
        drawList->AddLine(ImVec2(ruler.Min.x, ruler.Max.y - 0.5f), ImVec2(ruler.Max.x, ruler.Max.y - 0.5f), dividerColor);

        const float timeLeft  = timeline.Min.x + s_timelineInset;
        const float timeRight = timeline.Max.x - s_timelineInset;
        const double duration = std::max(m_fileDuration, 1e-6);

        const std::function<float(double)> time_to_x = [timeLeft, timeRight, duration](double time)
        { return timeLeft + static_cast<float>(std::clamp(time / duration, 0.0, 1.0)) * (timeRight - timeLeft); };
        const auto x_to_time = [timeLeft, timeRight, duration](float x)
        { return static_cast<double>(std::clamp((x - timeLeft) / std::max(timeRight - timeLeft, 1.0f), 0.0f, 1.0f)) * duration; };

        drawList->PushClipRect(ruler.Min, ruler.Max, true);
        {
            const float labelSize        = theme::textSizeCaption;
            const double pixelsPerSecond = (timeRight - timeLeft) / duration;
            const ruler_step finestStep  = pick_ruler_step(pixelsPerSecond, calc_tabular_text_width(font, labelSize, "00:00") + 16.0f);
            const ruler_step step        = finestStep.major >= 1.0 ? finestStep : ruler_step{1.0, 4};  //< Labels are whole seconds
            const double minorStep       = step.major / step.minorDivisions;
            const long long last         = static_cast<long long>(std::floor(duration / minorStep + s_frameEpsilon));

            for (long long index = 0; index <= last && index < 4000; ++index)
            {
                const double time  = index * minorStep;
                const float x      = std::floor(time_to_x(time)) + 0.5f;
                const bool isMajor = index % step.minorDivisions == 0;

                drawList->AddLine(ImVec2(x, ruler.Max.y - (isMajor ? 8.0f : 3.0f)), ImVec2(x, ruler.Max.y), ImGui::GetColorU32(isMajor ? theme::textHelper : theme::borderStrong01));

                if (isMajor)
                {
                    draw_tabular_text(drawList, font, labelSize, ImVec2(x + 3.0f, ruler.Min.y + 2.0f), ImGui::GetColorU32(theme::textSecondary), format_timecode(time, false));
                }
            }
        }
        drawList->PopClipRect();

        const ImRect track(ImVec2(timeLeft, std::floor(scrubRow.GetCenter().y) - 2.0f), ImVec2(timeRight, std::floor(scrubRow.GetCenter().y) + 2.0f));
        const float playheadX = std::floor(time_to_x(m_filePosition)) + 0.5f;
        drawList->AddRectFilled(track.Min, track.Max, ImGui::GetColorU32(theme::layer03), 2.0f);
        drawList->AddRectFilled(track.Min, ImVec2(playheadX, track.Max.y), ImGui::GetColorU32(theme::interactive), 2.0f);

        render_timeline_markers(markerLane, time_to_x);

        // The ruler and scrubber both seek; the marker lane is left to the markers so they stay clickable.
        bool scrubbing = false;
        bool hovered   = false;
        for (const auto& [id, rect] : {std::pair{"##VideoRuler", ruler}, std::pair{"##VideoScrub", scrubRow}})
        {
            ImGui::SetCursorScreenPos(rect.Min);
            ImGui::InvisibleButton(id, rect.GetSize());
            scrubbing |= ImGui::IsItemActive();
            hovered |= ImGui::IsItemHovered();
        }

        if (scrubbing)
        {
            seek_to_position(x_to_time(ImGui::GetMousePos().x));
        }

        if (hovered || scrubbing)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }

        if (hovered && !scrubbing)
        {
            const float x               = std::floor(std::clamp(ImGui::GetMousePos().x, timeLeft, timeRight)) + 0.5f;
            const std::string hoverTime = format_time(x_to_time(x));
            const float labelSize       = theme::textSizeCaption;
            const float width           = calc_tabular_text_width(font, labelSize, hoverTime) + 8.0f;
            constexpr float s_cursorGap = 6.0f;
            const float left            = x + s_cursorGap + width > ruler.Max.x ? x - s_cursorGap - width : x + s_cursorGap;

            drawList->AddLine(ImVec2(x, markerLane.Min.y), ImVec2(x, scrubRow.Max.y), ImGui::GetColorU32(theme::adjust_alpha(theme::textPrimary, 0.25f)));
            drawList->AddRectFilled(ImVec2(left, ruler.Min.y + 1.0f), ImVec2(left + width, ruler.Max.y - 1.0f), ImGui::GetColorU32(theme::layer03), 2.0f);
            draw_tabular_text(drawList, font, labelSize, ImVec2(left + 4.0f, ruler.Min.y + 2.0f), ImGui::GetColorU32(theme::textPrimary), hoverTime);
        }

        const ImU32 playheadColor = ImGui::GetColorU32(theme::textPrimary);
        drawList->AddLine(ImVec2(playheadX, ruler.Min.y), ImVec2(playheadX, scrubRow.Max.y), playheadColor);

        constexpr float s_handleHalfWidth = 5.5f;
        const ImVec2 handle[]             = {ImVec2(playheadX - s_handleHalfWidth, ruler.Min.y),
                                             ImVec2(playheadX + s_handleHalfWidth, ruler.Min.y),
                                             ImVec2(playheadX + s_handleHalfWidth, ruler.Min.y + 7.0f),
                                             ImVec2(playheadX, ruler.Min.y + 12.0f),
                                             ImVec2(playheadX - s_handleHalfWidth, ruler.Min.y + 7.0f)};
        drawList->AddConvexPolyFilled(handle, IM_ARRAYSIZE(handle), playheadColor);
        drawList->AddCircleFilled(ImVec2(playheadX, track.GetCenter().y), 6.0f, playheadColor);
    }

    auto video_element::render_status_bar(const ImRect& statusBar) -> void
    {
        ImDrawList* const drawList = ImGui::GetWindowDrawList();
        ImFont* const font         = text_font();

        const float textSize     = theme::textSizeHelper;
        const float textY        = statusBar.Min.y + (statusBar.GetHeight() - textSize) * 0.5f - 1.0f;
        const float cellPadding  = theme::space08 + 2.0f;
        const ImU32 labelColor   = ImGui::GetColorU32(theme::textHelper);
        const ImU32 valueColor   = ImGui::GetColorU32(theme::textSecondary);
        const ImU32 dividerColor = ImGui::GetColorU32(theme::background);

        drawList->AddRectFilled(statusBar.Min, statusBar.Max, ImGui::GetColorU32(theme::layer02), theme::radiusSm, ImDrawFlags_RoundCornersBottom);
        drawList->AddLine(ImVec2(statusBar.Min.x, statusBar.Min.y + 0.5f), ImVec2(statusBar.Max.x, statusBar.Min.y + 0.5f), dividerColor);

        const video_info* const info = get_info();
        const double frameRate       = info ? info->frameRate : 0.0;

        const std::string summary = frameRate > 0.0 ? fmt::format("{}  {} frames", format_time(m_fileDuration), frame_at(m_fileDuration, frameRate)) : format_timecode(m_fileDuration);
        const float summaryWidth  = calc_tabular_text_width(font, textSize, summary);
        const float summaryLeft   = statusBar.Max.x - summaryWidth - cellPadding;

        drawList->PushClipRect(statusBar.Min, ImVec2(std::max(summaryLeft - cellPadding, statusBar.Min.x), statusBar.Max.y), true);

        float x = statusBar.Min.x;

        const auto draw_cell = [&](const char* label, const std::string& value, const char* unit)
        {
            x += cellPadding;
            drawList->AddText(font, textSize, ImVec2(x, textY), labelColor, label);
            x += font->CalcTextSizeA(textSize, FLT_MAX, 0.0f, label).x + 5.0f;
            x += draw_tabular_text(drawList, font, textSize, ImVec2(x, textY), valueColor, value);
            if (unit)
            {
                x += 4.0f;
                drawList->AddText(font, textSize, ImVec2(x, textY), labelColor, unit);
                x += font->CalcTextSizeA(textSize, FLT_MAX, 0.0f, unit).x;
            }
            x += cellPadding;
            drawList->AddLine(ImVec2(std::floor(x) + 0.5f, statusBar.Min.y + 1.0f), ImVec2(std::floor(x) + 0.5f, statusBar.Max.y), dividerColor);
        };

        if (info && info->displayWidth > 0)
        {
            draw_cell("Size", fmt::format("{}x{}", info->displayWidth, info->displayHeight), nullptr);
        }
        if (frameRate > 0.0)
        {
            draw_cell("Rate", format_frame_rate(frameRate), "fps");
        }
        if (info && !info->videoCodec.empty())
        {
            draw_cell("Video", format_codec(info->videoCodec), nullptr);
        }
        if (info && info->fileSize > 0 && m_fileDuration > 0.0)
        {
            draw_cell("Bitrate", fmt::format("{:.1f}", static_cast<double>(info->fileSize) * 8.0 / m_fileDuration / 1e6), "Mb/s");
        }
        if (info && !info->audioCodec.empty())
        {
            const std::string audio = info->audioSampleRate > 0 ? fmt::format("{}  {:g} kHz  {} ch", format_codec(info->audioCodec), info->audioSampleRate / 1000.0, info->audioChannels)
                                                                : format_codec(info->audioCodec);
            draw_cell("Audio", audio, nullptr);
        }
        else if (info && !info->videoCodec.empty())
        {
            draw_cell("Audio", "None", nullptr);
        }
        if (info && info->fileSize > 0)
        {
            draw_cell("File", format_megabytes(info->fileSize), "MB");
        }

        drawList->PopClipRect();

        if (summaryLeft > statusBar.Min.x)
        {
            draw_tabular_text(drawList, font, textSize, ImVec2(summaryLeft, textY), labelColor, summary);
        }
    }

    auto video_element::get_file_play_position() const -> double
    {
        return m_videoSubsystem.lock()->get_video_play_position(m_filePath);
    }

    auto video_element::get_file_duration() const -> double
    {
        return m_videoSubsystem.lock()->get_video_duration(m_filePath);
    }

    auto video_element::play_file() -> void
    {
        m_videoSubsystem.lock()->play_video(m_filePath);
    }

    auto video_element::pause_file() -> void
    {
        m_videoSubsystem.lock()->pause_video(m_filePath);
    }

    auto video_element::stop_file() -> void
    {
        pause_file();
        seek_to_position(0.0);
    }

    auto video_element::seek_to_position(double position) -> void
    {
        m_videoSubsystem.lock()->set_video_play_position(m_filePath, position);
    }

    auto video_element::prev_frame() -> void
    {
        m_videoSubsystem.lock()->set_video_prev_frame(m_filePath);
    }

    auto video_element::next_frame() -> void
    {
        m_videoSubsystem.lock()->set_video_next_frame(m_filePath);
    }

    auto video_element::get_is_playing() -> bool
    {
        return m_videoSubsystem.lock()->get_video_is_playing(m_filePath);
    }
}  // namespace gluten
