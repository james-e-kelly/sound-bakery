#include "video_controls.h"

#include "IconsFontaudio.h"
#include "IconsLucide.h"
#include "gluten/app/app.h"
#include "gluten/elements/audio_element.h"
#include "gluten/elements/video_element.h"
#include "gluten/subsystems/audio_subsystem.h"
#include "imgui_internal.h"

namespace
{
    constexpr float s_barPadding      = 4.0f;
    constexpr float s_groupSpacing    = 8.0f;
    constexpr float s_playButtonWidth = 30.0f;
    constexpr int s_playbackButtons   = 5;  //< Stop, previous frame, play/pause, next frame, loop

    constexpr const char* s_timecodeLabel = "Timecode";
    constexpr const char* s_framesLabel   = "Frames";
    constexpr const char* s_burnInLabel   = "TC";

    auto text_font() -> ImFont* { return gluten::app::get()->get_font(gluten::fonts::regular); }
    auto icon_font() -> ImFont* { return gluten::app::get()->get_font(gluten::fonts::regular_lucide_icons); }
    auto audio_icon_font() -> ImFont* { return gluten::app::get()->get_font(gluten::fonts::regular_audio_icons); }

    auto text_button_width(const char* label) -> float
    {
        return std::ceil(text_font()->CalcTextSizeA(gluten::theme::textSizeHelper, FLT_MAX, 0.0f, label).x) + gluten::theme::space08 * 2.0f + 4.0f;
    }

    auto timecode_width() -> float
    {
        return gluten::calc_tabular_text_width(text_font(), gluten::theme::textSizeBody, "00:00:00:00") +
               gluten::calc_tabular_text_width(text_font(), gluten::theme::textSizeHelper, " / 00:00:00:00");
    }

    auto display_toggle_width() -> float { return text_button_width(s_timecodeLabel) + text_button_width(s_framesLabel) - 1.0f; }
    auto overlay_toggles_width() -> float { return gluten::audio_transport::s_buttonSize * 2.0f + text_button_width(s_burnInLabel) - 2.0f; }

    struct transport_widths
    {
        float left;
        float toggles;
        float right;
    };

    auto measure_transport(float extrasWidth) -> transport_widths
    {
        const float extras = extrasWidth > 0.0f ? extrasWidth + s_groupSpacing : 0.0f;

        transport_widths widths;
        widths.left    = s_barPadding + s_playButtonWidth * s_playbackButtons - (s_playbackButtons - 1) + s_groupSpacing + timecode_width() + s_groupSpacing + extras;
        widths.toggles = display_toggle_width() + s_groupSpacing + overlay_toggles_width();
        widths.right   = gluten::get_preview_speed_width() + s_groupSpacing + gluten::get_preview_volume_width() + s_barPadding;
        return widths;
    }

    auto fits_one_row(float width, const transport_widths& widths) -> bool
    {
        return widths.left + widths.toggles + s_groupSpacing + widths.right <= width;
    }

    /**
     * @brief Stop, frame step, play/pause, loop and the timecode, from the left. Returns where they end.
     */
    auto render_playback(float x, float top, float centerY, gluten::video_element& video, gluten::audio_subsystem* audioSubsystem) -> float
    {
        const float buttonSize = gluten::audio_transport::s_buttonSize;
        const auto next_rect   = [&x, top, buttonSize]()
        { return ImRect(x, top, x + s_playButtonWidth, top + buttonSize); };

        if (gluten::transport_button("##Stop", next_rect(), ICON_FAD_STOP, audio_icon_font(), gluten::theme::iconSizeMd, false, ImDrawFlags_RoundCornersLeft))
        {
            video.stop_file();
        }
        ImGui::SetItemTooltip("Stop");
        x += s_playButtonWidth - 1.0f;

        if (gluten::transport_button("##PrevFrame", next_rect(), ICON_LC_STEP_BACK, icon_font(), gluten::theme::iconSizeSm, false, ImDrawFlags_RoundCornersNone))
        {
            video.pause_file();
            video.prev_frame();
        }
        ImGui::SetItemTooltip("Previous frame");
        x += s_playButtonWidth - 1.0f;

        const bool playing = video.get_is_playing();
        if (gluten::transport_button("##PlayPause", next_rect(), playing ? ICON_FAD_PAUSE : ICON_FAD_PLAY, audio_icon_font(), gluten::theme::iconSizeMd, playing, ImDrawFlags_RoundCornersNone))
        {
            playing ? video.pause_file() : video.play_file();
        }
        ImGui::SetItemTooltip(playing ? "Pause (Space)" : "Play (Space)");
        x += s_playButtonWidth - 1.0f;

        if (gluten::transport_button("##NextFrame", next_rect(), ICON_LC_STEP_FORWARD, icon_font(), gluten::theme::iconSizeSm, false, ImDrawFlags_RoundCornersNone))
        {
            video.pause_file();
            video.next_frame();
        }
        ImGui::SetItemTooltip("Next frame");
        x += s_playButtonWidth - 1.0f;

        const bool looping = audioSubsystem && audioSubsystem->get_preview_looping();
        if (gluten::transport_button("##Loop", next_rect(), ICON_FAD_LOOP, audio_icon_font(), gluten::theme::iconSizeMd, looping, ImDrawFlags_RoundCornersRight, audioSubsystem != nullptr))
        {
            audioSubsystem->set_preview_looping(!looping);
        }
        ImGui::SetItemTooltip(looping ? "Stop looping" : "Loop");
        x += s_playButtonWidth + s_groupSpacing;

        ImDrawList* const drawList = ImGui::GetWindowDrawList();
        const std::string position = video.format_time(video.get_file_position());
        const std::string duration = fmt::format(" / {}", video.format_time(video.get_file_duration()));
        const float textY          = std::floor(centerY - gluten::theme::textSizeBody * 0.5f - 1.0f);

        const float positionWidth = gluten::draw_tabular_text(drawList, text_font(), gluten::theme::textSizeBody, ImVec2(x, textY), ImGui::GetColorU32(gluten::theme::textPrimary), position);
        gluten::draw_tabular_text(drawList, text_font(), gluten::theme::textSizeHelper, ImVec2(x + positionWidth, textY + 2.0f), ImGui::GetColorU32(gluten::theme::textHelper), duration);

        return x + timecode_width() + s_groupSpacing;
    }

    /**
     * @brief Timecode | Frames, then safe areas | TC | markers, ending at right. Returns where they start.
     */
    auto render_view_toggles(float right, float top) -> float
    {
        const float buttonSize = gluten::audio_transport::s_buttonSize;

        const auto toggle_button = [top, buttonSize](const char* id, float left, float width, const char* label, ImFont* font, float fontSize, bool active, ImDrawFlags corners)
        {
            return gluten::transport_button(id, ImRect(left, top, left + width, top + buttonSize), label, font, fontSize, active, corners);
        };

        const float burnInWidth = text_button_width(s_burnInLabel);
        float left              = right - buttonSize;

        const bool showMarkers = gluten::video_element::get_show_frame_markers();
        if (toggle_button("##FrameMarkers", left, buttonSize, ICON_LC_MESSAGE_SQUARE, icon_font(), gluten::theme::iconSizeSm, showMarkers, ImDrawFlags_RoundCornersRight))
        {
            gluten::video_element::set_show_frame_markers(!showMarkers);
        }
        ImGui::SetItemTooltip(showMarkers ? "Hide the marker at the playhead" : "Show the marker at the playhead on the frame");

        left -= burnInWidth - 1.0f;
        const bool showBurnIn = gluten::video_element::get_show_timecode_burn_in();
        if (toggle_button("##BurnIn", left, burnInWidth, s_burnInLabel, text_font(), gluten::theme::textSizeHelper, showBurnIn, ImDrawFlags_RoundCornersNone))
        {
            gluten::video_element::set_show_timecode_burn_in(!showBurnIn);
        }
        ImGui::SetItemTooltip("Timecode on the frame");

        left -= buttonSize - 1.0f;
        const bool showSafeAreas = gluten::video_element::get_show_safe_areas();
        if (toggle_button("##SafeAreas", left, buttonSize, ICON_LC_SCAN, icon_font(), gluten::theme::iconSizeSm, showSafeAreas, ImDrawFlags_RoundCornersLeft))
        {
            gluten::video_element::set_show_safe_areas(!showSafeAreas);
        }
        ImGui::SetItemTooltip("Action (93%%) and title (90%%) safe areas");

        const float framesWidth   = text_button_width(s_framesLabel);
        const float timecodeWidth = text_button_width(s_timecodeLabel);
        const bool showFrames     = gluten::video_element::get_time_display() == gluten::video_time_display::frames;

        left -= s_groupSpacing + framesWidth;
        if (toggle_button("##ShowFrames", left, framesWidth, s_framesLabel, text_font(), gluten::theme::textSizeHelper, showFrames, ImDrawFlags_RoundCornersRight))
        {
            gluten::video_element::set_time_display(gluten::video_time_display::frames);
        }
        ImGui::SetItemTooltip("Show times as frame numbers");

        left -= timecodeWidth - 1.0f;
        if (toggle_button("##ShowTimecode", left, timecodeWidth, s_timecodeLabel, text_font(), gluten::theme::textSizeHelper, !showFrames, ImDrawFlags_RoundCornersLeft))
        {
            gluten::video_element::set_time_display(gluten::video_time_display::timecode);
        }
        ImGui::SetItemTooltip("Show times as HH:MM:SS:FF");

        return left;
    }
}  // namespace

auto gluten::video_transport::get_height(float width, float extrasWidth) -> float
{
    return fits_one_row(width, measure_transport(extrasWidth)) ? audio_transport::s_rowHeight : audio_transport::s_rowHeight * 2.0f + theme::space04;
}

auto gluten::video_transport::render(const ImRect& area, video_element& video, const transport_extras& extras) -> void
{
    std::shared_ptr<audio_subsystem> audioSubsystem = app::get()->get_subsystem_by_class<audio_subsystem>();

    const transport_widths widths = measure_transport(extras.width);
    const bool oneRow             = fits_one_row(area.GetWidth(), widths);
    const float rowHeight         = audio_transport::s_rowHeight;
    const float buttonSize        = audio_transport::s_buttonSize;

    const ImRect firstRow(area.Min, ImVec2(area.Max.x, area.Min.y + rowHeight));
    const ImRect secondRow = oneRow ? firstRow : ImRect(ImVec2(area.Min.x, firstRow.Max.y + theme::space04), ImVec2(area.Max.x, firstRow.Max.y + theme::space04 + rowHeight));

    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(firstRow.Min, firstRow.Max, ImGui::GetColorU32(theme::layer02), theme::radiusSm);
    if (!oneRow)
    {
        drawList->AddRectFilled(secondRow.Min, secondRow.Max, ImGui::GetColorU32(theme::layer02), theme::radiusSm);
    }

    const auto button_top = [buttonSize](const ImRect& row)
    { return std::floor(row.Min.y + (row.GetHeight() - buttonSize) * 0.5f); };

    float left = render_playback(firstRow.Min.x + s_barPadding, button_top(firstRow), firstRow.GetCenter().y, video, audioSubsystem.get());

    if (extras.render && extras.width > 0.0f)
    {
        extras.render(ImRect(ImVec2(left, button_top(firstRow)), ImVec2(left + extras.width, button_top(firstRow) + buttonSize)));
        left += extras.width + s_groupSpacing;
    }

    float right = render_preview_volume(secondRow.Max.x - s_barPadding, button_top(secondRow), secondRow.GetCenter().y);
    right       = render_preview_speed(right - s_groupSpacing, secondRow.GetCenter().y);

    const float togglesRight = oneRow ? right - s_groupSpacing : firstRow.Max.x - s_barPadding;
    if (togglesRight - widths.toggles >= left)
    {
        render_view_toggles(togglesRight, button_top(firstRow));
    }
}
