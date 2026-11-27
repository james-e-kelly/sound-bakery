#include "audio_controls.h"

#include "IconsFontaudio.h"
#include "IconsLucide.h"
#include "gluten/app/app.h"
#include "gluten/elements/audio_element.h"
#include "gluten/subsystems/audio_subsystem.h"
#include "imgui_internal.h"

#include <array>
#include <cstring>
#include <span>

namespace
{
    constexpr float s_barPadding       = 4.0f;
    constexpr float s_groupSpacing     = 8.0f;
    constexpr float s_innerSpacing     = 6.0f;
    constexpr float s_playButtonWidth  = 30.0f;
    constexpr float s_faderWidth       = 96.0f;
    constexpr float s_speedWidth       = 72.0f;
    constexpr float s_meterWidth       = 48.0f;
    constexpr float s_volumeMaxDb      = 12.0f;
    constexpr float s_defaultVolumeDb  = 0.0f;
    constexpr float s_wheelStepDb      = 0.5f;
    constexpr float s_wheelStepSpeed   = 0.05f;
    constexpr float s_speedSnapRange   = 0.04f;  //< Dragging this close to 1x lands exactly on it
    constexpr float s_meterMinDb       = -60.0f;
    constexpr float s_meterMaxDb       = 0.0f;
    constexpr float s_meterYellowDb    = -12.0f;
    constexpr float s_meterRedDb       = -3.0f;
    constexpr float s_meterFallDbPerSec = 24.0f;
    constexpr float s_gridCellMinWidth = 150.0f;

    constexpr float s_volumeMinDb = gluten::audio_subsystem::s_previewVolumeMinDb;
    constexpr float s_speedMin    = gluten::audio_subsystem::s_previewSpeedMin;
    constexpr float s_speedMax    = gluten::audio_subsystem::s_previewSpeedMax;

    bool s_channelGridOpen = false;

    auto get_audio_subsystem() -> std::shared_ptr<gluten::audio_subsystem>
    {
        return gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>();
    }

    auto text_font() -> ImFont* { return gluten::app::get()->get_font(gluten::fonts::regular); }
    auto icon_font() -> ImFont* { return gluten::app::get()->get_font(gluten::fonts::regular_lucide_icons); }
    auto audio_icon_font() -> ImFont* { return gluten::app::get()->get_font(gluten::fonts::regular_audio_icons); }

    auto linear_to_db(float linear) -> float { return linear > 0.0f ? 20.0f * std::log10(linear) : -std::numeric_limits<float>::infinity(); }

    /**
     * @brief Console-style fader taper: most of the travel covers -18 dB to +12 dB, where previews are actually set.
     */
    constexpr std::pair<float, float> s_faderTaper[] = {{0.0f, s_volumeMinDb}, {0.15f, -36.0f}, {0.35f, -18.0f}, {0.75f, 0.0f}, {1.0f, s_volumeMaxDb}};

    auto fader_position_to_db(float position) -> float
    {
        for (std::size_t index = 1; index < std::size(s_faderTaper); ++index)
        {
            const auto& [endPosition, endDb]     = s_faderTaper[index];
            const auto& [startPosition, startDb] = s_faderTaper[index - 1];
            if (position <= endPosition)
            {
                return startDb + (position - startPosition) / (endPosition - startPosition) * (endDb - startDb);
            }
        }
        return s_volumeMaxDb;
    }

    auto db_to_fader_position(float db) -> float
    {
        for (std::size_t index = 1; index < std::size(s_faderTaper); ++index)
        {
            const auto& [endPosition, endDb]     = s_faderTaper[index];
            const auto& [startPosition, startDb] = s_faderTaper[index - 1];
            if (db <= endDb)
            {
                return startPosition + std::max(db - startDb, 0.0f) / (endDb - startDb) * (endPosition - startPosition);
            }
        }
        return 1.0f;
    }

    auto text_button_width(const char* label) -> float
    {
        return std::ceil(text_font()->CalcTextSizeA(gluten::theme::textSizeHelper, FLT_MAX, 0.0f, label).x) + gluten::theme::space08 * 2.0f + 4.0f;
    }

    auto timecode_width() -> float
    {
        return gluten::calc_tabular_text_width(text_font(), gluten::theme::textSizeBody, "00:00.000") + gluten::calc_tabular_text_width(text_font(), gluten::theme::textSizeHelper, " / 00:00.000");
    }

    auto volume_readout_width() -> float { return gluten::calc_tabular_text_width(text_font(), gluten::theme::textSizeHelper, "+00.0 dB"); }
    auto speed_readout_width() -> float { return gluten::calc_tabular_text_width(text_font(), gluten::theme::textSizeHelper, "0.00x"); }

    constexpr const char* s_loudnessLabel = "Loudness";
    constexpr const char* s_bandsLabel    = "Bands";

    struct transport_widths
    {
        float left;
        float toggles;
        float speed;
        float volume;
    };

    auto measure_transport(float extrasWidth) -> transport_widths
    {
        const float extras = extrasWidth > 0.0f ? extrasWidth + s_groupSpacing : 0.0f;

        transport_widths widths;
        widths.left    = s_barPadding + s_playButtonWidth * 3.0f - 2.0f + s_groupSpacing + timecode_width() + s_groupSpacing + extras;
        widths.toggles = text_button_width(s_loudnessLabel) + text_button_width(s_bandsLabel) - 1.0f;
        widths.speed   = s_speedWidth + s_innerSpacing + speed_readout_width();
        widths.volume  = gluten::audio_transport::s_buttonSize + s_innerSpacing + s_faderWidth + s_innerSpacing + volume_readout_width() + s_groupSpacing + s_meterWidth + s_barPadding;
        return widths;
    }

    auto fits_one_row(float width, const transport_widths& widths) -> bool
    {
        return widths.left + widths.toggles + s_groupSpacing + widths.speed + s_groupSpacing + widths.volume <= width;
    }

    /**
     * @brief Stop, play/pause, loop and the timecode, from the left. Returns where they end.
     */
    auto render_playback(float x, float top, float centerY, gluten::audio_element& waveform, gluten::audio_subsystem& audioSubsystem) -> float
    {
        ImDrawList* const drawList = ImGui::GetWindowDrawList();
        const float buttonSize     = gluten::audio_transport::s_buttonSize;

        if (gluten::transport_button("##Stop", ImRect(x, top, x + s_playButtonWidth, top + buttonSize), ICON_FAD_STOP, audio_icon_font(), gluten::theme::iconSizeMd, false, ImDrawFlags_RoundCornersLeft))
        {
            waveform.pause();
            waveform.seek(0.0);
        }
        ImGui::SetItemTooltip("Stop");
        x += s_playButtonWidth - 1.0f;

        const bool playing = waveform.is_playing();
        if (gluten::transport_button("##PlayPause", ImRect(x, top, x + s_playButtonWidth, top + buttonSize), playing ? ICON_FAD_PAUSE : ICON_FAD_PLAY, audio_icon_font(), gluten::theme::iconSizeMd, playing, ImDrawFlags_RoundCornersNone))
        {
            playing ? waveform.pause() : waveform.play();
        }
        ImGui::SetItemTooltip(playing ? "Pause (Space)" : "Play (Space)");
        x += s_playButtonWidth - 1.0f;

        const bool looping = audioSubsystem.get_preview_looping();
        if (gluten::transport_button("##Loop", ImRect(x, top, x + s_playButtonWidth, top + buttonSize), ICON_FAD_LOOP, audio_icon_font(), gluten::theme::iconSizeMd, looping, ImDrawFlags_RoundCornersRight))
        {
            audioSubsystem.set_preview_looping(!looping);
        }
        ImGui::SetItemTooltip(looping ? "Stop looping" : "Loop");
        x += s_playButtonWidth + s_groupSpacing;

        const std::string position = gluten::format_timecode(waveform.get_position());
        const std::string duration = fmt::format(" / {}", gluten::format_timecode(waveform.get_duration()));
        const float textY          = std::floor(centerY - gluten::theme::textSizeBody * 0.5f - 1.0f);

        const float positionWidth = gluten::draw_tabular_text(drawList, text_font(), gluten::theme::textSizeBody, ImVec2(x, textY), ImGui::GetColorU32(gluten::theme::textPrimary), position);
        gluten::draw_tabular_text(drawList, text_font(), gluten::theme::textSizeHelper, ImVec2(x + positionWidth, textY + 2.0f), ImGui::GetColorU32(gluten::theme::textHelper), duration);

        return x + timecode_width() + s_groupSpacing;
    }

    /**
     * @brief Loudness | Bands overlay toggles, ending at right. Returns where they start.
     */
    auto render_overlay_toggles(float right, float top, gluten::audio_element& waveform) -> float
    {
        const float buttonSize   = gluten::audio_transport::s_buttonSize;
        const float loudnessWidth = text_button_width(s_loudnessLabel);
        const float bandsWidth    = text_button_width(s_bandsLabel);

        const gluten::waveform_overlay overlay = waveform.get_overlay();
        const auto toggle                      = [&waveform, overlay](gluten::waveform_overlay target)
        {
            waveform.set_overlay(overlay == target ? gluten::waveform_overlay::none : target);
        };

        const ImRect bandsRect(ImVec2(right - bandsWidth, top), ImVec2(right, top + buttonSize));
        const ImRect loudnessRect(ImVec2(bandsRect.Min.x - loudnessWidth + 1.0f, top), ImVec2(bandsRect.Min.x + 1.0f, top + buttonSize));

        if (gluten::transport_button("##LoudnessOverlay", loudnessRect, s_loudnessLabel, text_font(), gluten::theme::textSizeHelper, overlay == gluten::waveform_overlay::loudness, ImDrawFlags_RoundCornersLeft))
        {
            toggle(gluten::waveform_overlay::loudness);
        }
        ImGui::SetItemTooltip("Short-term and integrated loudness");

        if (gluten::transport_button("##BandsOverlay", bandsRect, s_bandsLabel, text_font(), gluten::theme::textSizeHelper, overlay == gluten::waveform_overlay::bands, ImDrawFlags_RoundCornersRight))
        {
            toggle(gluten::waveform_overlay::bands);
        }
        ImGui::SetItemTooltip("Low, mid and high band levels");

        return loudnessRect.Min.x;
    }

    struct slider_interaction
    {
        bool hovered;
        bool dragging;
        bool resetRequested;
    };

    /**
     * @brief Mouse handling shared by the volume and speed sliders.
     *
     * A double-click asks for a reset and releases the slider, as its second press would otherwise keep
     * dragging the value back to the mouse.
     */
    auto slider_interaction_for(const char* id, const ImRect& rect) -> slider_interaction
    {
        ImGui::SetCursorScreenPos(rect.Min);
        ImGui::InvisibleButton(id, rect.GetSize());

        slider_interaction interaction{ImGui::IsItemHovered(), ImGui::IsItemActive(), false};

        if (interaction.hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            interaction.resetRequested = true;
            interaction.dragging       = false;

            if (ImGui::IsItemActive())
            {
                ImGui::ClearActiveID();
            }
        }

        return interaction;
    }

    auto draw_slider(const ImRect& slider, float handlePosition, float markPosition, bool highlighted, float fillFrom) -> void
    {
        ImDrawList* const drawList = ImGui::GetWindowDrawList();
        const float centerY        = slider.GetCenter().y;
        const auto to_x            = [&slider](float position) { return std::floor(slider.Min.x + position * slider.GetWidth()); };

        const float handleX = to_x(handlePosition);
        const float markX   = to_x(markPosition) + 0.5f;
        const ImRect track(ImVec2(slider.Min.x, centerY - 2.0f), ImVec2(slider.Max.x, centerY + 2.0f));

        drawList->AddRectFilled(track.Min, track.Max, ImGui::GetColorU32(gluten::theme::background));
        drawList->AddRectFilled(ImVec2(std::min(to_x(fillFrom), handleX), track.Min.y), ImVec2(std::max(to_x(fillFrom), handleX), track.Max.y), ImGui::GetColorU32(gluten::theme::textHelper));
        drawList->AddRect(track.Min, track.Max, ImGui::GetColorU32(gluten::theme::borderSubtle01));
        drawList->AddLine(ImVec2(markX, centerY - 6.0f), ImVec2(markX, centerY - 3.0f), ImGui::GetColorU32(gluten::theme::textHelper));
        drawList->AddLine(ImVec2(markX, centerY + 3.0f), ImVec2(markX, centerY + 6.0f), ImGui::GetColorU32(gluten::theme::textHelper));
        drawList->AddRectFilled(ImVec2(handleX - 3.0f, centerY - 7.0f), ImVec2(handleX + 4.0f, centerY + 7.0f), ImGui::GetColorU32(highlighted ? gluten::theme::textPrimary : gluten::theme::textSecondary), 1.0f);
        drawList->AddRect(ImVec2(handleX - 3.0f, centerY - 7.0f), ImVec2(handleX + 4.0f, centerY + 7.0f), ImGui::GetColorU32(gluten::theme::background), 1.0f);
    }

    auto speed_to_position(float speed) -> float { return (speed - s_speedMin) / (s_speedMax - s_speedMin); }

    /**
     * @brief Speed slider and readout, ending at right. Returns where they start.
     */
    auto render_speed(float right, float centerY, gluten::audio_subsystem& audioSubsystem) -> float
    {
        ImDrawList* const drawList = ImGui::GetWindowDrawList();
        float speed                = audioSubsystem.get_preview_speed();

        const float readoutWidth = speed_readout_width();
        gluten::draw_tabular_text(drawList, text_font(), gluten::theme::textSizeHelper, ImVec2(right - readoutWidth, std::floor(centerY - gluten::theme::textSizeHelper * 0.5f - 1.0f)),
                                  ImGui::GetColorU32(speed == 1.0f ? gluten::theme::textHelper : gluten::theme::textSecondary), fmt::format("{:.2f}x", speed));
        right -= readoutWidth + s_innerSpacing;

        const ImRect slider(ImVec2(right - s_speedWidth, centerY - 8.0f), ImVec2(right, centerY + 8.0f));
        const slider_interaction interaction = slider_interaction_for("##Speed", slider);

        float newSpeed = speed;
        if (interaction.dragging)
        {
            const float position = std::clamp((ImGui::GetMousePos().x - slider.Min.x) / slider.GetWidth(), 0.0f, 1.0f);
            newSpeed             = s_speedMin + position * (s_speedMax - s_speedMin);
            if (std::abs(newSpeed - 1.0f) < s_speedSnapRange)
            {
                newSpeed = 1.0f;
            }
        }
        if (interaction.hovered && ImGui::GetIO().MouseWheel != 0.0f)
        {
            newSpeed = std::round((speed + ImGui::GetIO().MouseWheel * s_wheelStepSpeed) / s_wheelStepSpeed) * s_wheelStepSpeed;
        }
        if (interaction.resetRequested)
        {
            newSpeed = 1.0f;
        }
        ImGui::SetItemTooltip("Playback speed (pitch changes with it). Scroll to nudge, double-click for 1x.");

        if (newSpeed != speed)
        {
            audioSubsystem.set_preview_speed(newSpeed);
            speed = audioSubsystem.get_preview_speed();
        }

        draw_slider(slider, speed_to_position(speed), speed_to_position(1.0f), interaction.hovered || interaction.dragging, speed_to_position(1.0f));

        return slider.Min.x;
    }

    /**
     * @brief Mute, volume fader and output meter, ending at right. Returns where they start.
     */
    auto render_volume(float right, float top, float centerY, gluten::audio_element& waveform, gluten::audio_subsystem& audioSubsystem) -> float
    {
        ImDrawList* const drawList = ImGui::GetWindowDrawList();
        const float previewVolume  = audioSubsystem.get_preview_volume();

        {
            static std::unordered_map<std::filesystem::path, std::vector<float>> s_meterLevelsDb;

            const std::size_t channels = waveform.get_channel_count();
            const std::size_t rows     = channels == 2 ? 2 : 1;
            const std::vector<float> peaks = waveform.is_playing() ? waveform.get_channel_peaks_at(waveform.get_position()) : std::vector<float>();

            std::vector<float>& meterLevelsDb = s_meterLevelsDb[waveform.get_file_path()];
            meterLevelsDb.resize(rows, s_meterMinDb);

            const float fall = s_meterFallDbPerSec * ImGui::GetIO().DeltaTime;
            for (std::size_t row = 0; row < rows; ++row)
            {
                // Beyond stereo the meter shows the loudest channel, as a row per channel wouldn't fit.
                const float peak    = peaks.empty() ? 0.0f : (rows == 2 ? peaks[std::min(row, peaks.size() - 1)] : *std::max_element(peaks.begin(), peaks.end()));
                const float levelDb = std::max(linear_to_db(peak * previewVolume), s_meterMinDb);
                meterLevelsDb[row]  = std::max(levelDb, meterLevelsDb[row] - fall);
            }

            const float rowHeight = 4.0f;
            const float rowGap    = 2.0f;
            const float meterTop  = std::floor(centerY - (rows * rowHeight + (rows - 1) * rowGap) * 0.5f);
            const float meterLeft = right - s_meterWidth;

            const auto db_to_x = [&](float db) { return meterLeft + (db - s_meterMinDb) / (s_meterMaxDb - s_meterMinDb) * s_meterWidth; };

            for (std::size_t row = 0; row < rows; ++row)
            {
                const float rowTop = meterTop + row * (rowHeight + rowGap);
                drawList->AddRectFilled(ImVec2(meterLeft, rowTop), ImVec2(right, rowTop + rowHeight), ImGui::GetColorU32(gluten::theme::background));

                const float level                         = std::min(meterLevelsDb[row], s_meterMaxDb);
                const std::pair<float, ImVec4> segments[] = {{std::min(level, s_meterYellowDb), gluten::theme::supportSuccess},
                                                             {std::min(level, s_meterRedDb), gluten::theme::supportWarning},
                                                             {level, gluten::theme::supportError}};
                float segmentStart                        = s_meterMinDb;
                for (const auto& [segmentEnd, color] : segments)
                {
                    if (segmentEnd > segmentStart)
                    {
                        drawList->AddRectFilled(ImVec2(db_to_x(segmentStart), rowTop), ImVec2(db_to_x(segmentEnd), rowTop + rowHeight), ImGui::GetColorU32(color));
                        segmentStart = segmentEnd;
                    }
                }
            }

            ImGui::SetCursorScreenPos(ImVec2(meterLeft, meterTop - 2.0f));
            ImGui::InvisibleButton("##Meter", ImVec2(s_meterWidth, rows * rowHeight + rowGap + 4.0f));
            ImGui::SetItemTooltip(rows == 2 ? "Output level after the preview volume" : "Loudest channel's output level after the preview volume");

            right = meterLeft - s_groupSpacing;
        }

        const float volumeDb = audioSubsystem.get_preview_volume_db();
        const bool muted     = audioSubsystem.get_preview_muted();

        {
            const float readoutWidth  = volume_readout_width();
            const std::string readout = volumeDb <= s_volumeMinDb ? std::string("-inf dB") : fmt::format("{:+.1f} dB", volumeDb);
            gluten::draw_tabular_text(drawList, text_font(), gluten::theme::textSizeHelper, ImVec2(right - readoutWidth, std::floor(centerY - gluten::theme::textSizeHelper * 0.5f - 1.0f)),
                                      ImGui::GetColorU32(muted ? gluten::theme::textHelper : gluten::theme::textSecondary), readout);
            right -= readoutWidth + s_innerSpacing;
        }

        {
            const ImRect fader(ImVec2(right - s_faderWidth, centerY - 8.0f), ImVec2(right, centerY + 8.0f));
            const slider_interaction interaction = slider_interaction_for("##Volume", fader);

            float newVolumeDb = volumeDb;
            if (interaction.dragging)
            {
                newVolumeDb = fader_position_to_db(std::clamp((ImGui::GetMousePos().x - fader.Min.x) / fader.GetWidth(), 0.0f, 1.0f));
            }
            if (interaction.hovered && ImGui::GetIO().MouseWheel != 0.0f)
            {
                newVolumeDb = std::clamp(std::round((volumeDb + ImGui::GetIO().MouseWheel * s_wheelStepDb) / s_wheelStepDb) * s_wheelStepDb, s_volumeMinDb, s_volumeMaxDb);
            }
            if (interaction.resetRequested)
            {
                newVolumeDb = s_defaultVolumeDb;
            }
            ImGui::SetItemTooltip("Preview volume. Scroll to nudge, double-click to reset.");

            if (newVolumeDb != volumeDb)
            {
                audioSubsystem.set_preview_volume_db(newVolumeDb);
            }

            draw_slider(fader, db_to_fader_position(audioSubsystem.get_preview_volume_db()), db_to_fader_position(0.0f), interaction.hovered || interaction.dragging, 0.0f);

            right = fader.Min.x - s_innerSpacing;
        }

        const float buttonSize = gluten::audio_transport::s_buttonSize;
        const ImRect muteRect(ImVec2(right - buttonSize, top), ImVec2(right, top + buttonSize));
        if (gluten::transport_button("##Mute", muteRect, muted ? ICON_LC_VOLUME_X : ICON_LC_VOLUME_2, icon_font(), gluten::theme::iconSizeSm, muted, ImDrawFlags_RoundCornersAll, true, gluten::theme::supportWarning, gluten::theme::background))
        {
            audioSubsystem.set_preview_muted(!muted);
        }
        ImGui::SetItemTooltip(muted ? "Unmute" : "Mute");

        return muteRect.Min.x;
    }

    auto draw_text_in(ImDrawList* drawList, ImFont* font, float fontSize, const ImRect& rect, const ImVec4& color, std::string_view text, float alignX = 0.0f) -> void
    {
        const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text.data(), text.data() + text.size());
        const float x         = std::floor(rect.Min.x + std::max(rect.GetWidth() - textSize.x, 0.0f) * alignX);
        const float y         = std::floor(rect.GetCenter().y - textSize.y * 0.5f);

        drawList->PushClipRect(rect.Min, rect.Max, true);
        drawList->AddText(font, fontSize, ImVec2(x, y), ImGui::GetColorU32(color), text.data(), text.data() + text.size());
        drawList->PopClipRect();
    }

    auto grid_columns(float width) -> std::size_t
    {
        return std::max<std::size_t>(1, static_cast<std::size_t>(width / s_gridCellMinWidth));
    }
}  // namespace

auto gluten::transport_button(const char* id, const ImRect& rect, const char* label, ImFont* font, float fontSize, bool active, ImDrawFlags corners, bool enabled, const ImVec4& activeFill, const ImVec4& activeText) -> bool
{
    ImGui::SetCursorScreenPos(rect.Min);
    ImGui::BeginDisabled(!enabled);
    const bool pressed = ImGui::InvisibleButton(id, rect.GetSize());
    const bool hovered = ImGui::IsItemHovered();
    const bool held    = ImGui::IsItemActive();
    ImGui::EndDisabled();

    ImVec4 background = ImLerp(theme::layer02, theme::layer03, 0.35f);
    if (active)
    {
        background = activeFill;
    }
    else if (held)
    {
        background = theme::layerActive02;
    }
    else if (hovered && enabled)
    {
        background = theme::layerHover02;
    }

    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(rect.Min, rect.Max, ImGui::GetColorU32(background), theme::radiusSm, corners);
    drawList->AddRect(rect.Min, rect.Max, ImGui::GetColorU32(active ? activeFill : theme::borderStrong01), theme::radiusSm, corners);

    const ImVec4 textColor = !enabled ? theme::textDisabled : (active ? activeText : theme::textPrimary);
    const ImVec2 labelSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, label);
    drawList->AddText(font, fontSize, ImVec2(std::floor(rect.GetCenter().x - labelSize.x * 0.5f), std::floor(rect.GetCenter().y - labelSize.y * 0.5f)), ImGui::GetColorU32(textColor), label);

    return pressed && enabled;
}

auto gluten::audio_transport::get_height(float width, float extrasWidth) -> float
{
    return fits_one_row(width, measure_transport(extrasWidth)) ? s_rowHeight : s_rowHeight * 2.0f + theme::space04;
}

auto gluten::audio_transport::render(const ImRect& area, audio_element& waveform, const transport_extras& extras) -> void
{
    std::shared_ptr<gluten::audio_subsystem> audioSubsystem = get_audio_subsystem();
    if (!audioSubsystem)
    {
        return;
    }

    const transport_widths widths = measure_transport(extras.width);
    const bool oneRow             = fits_one_row(area.GetWidth(), widths);

    const ImRect firstRow(area.Min, ImVec2(area.Max.x, area.Min.y + s_rowHeight));
    const ImRect secondRow = oneRow ? firstRow : ImRect(ImVec2(area.Min.x, firstRow.Max.y + theme::space04), ImVec2(area.Max.x, firstRow.Max.y + theme::space04 + s_rowHeight));

    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(firstRow.Min, firstRow.Max, ImGui::GetColorU32(theme::layer02), theme::radiusSm);
    if (!oneRow)
    {
        drawList->AddRectFilled(secondRow.Min, secondRow.Max, ImGui::GetColorU32(theme::layer02), theme::radiusSm);
    }

    const auto button_top = [](const ImRect& row) { return std::floor(row.Min.y + (row.GetHeight() - s_buttonSize) * 0.5f); };

    float left = render_playback(firstRow.Min.x + s_barPadding, button_top(firstRow), firstRow.GetCenter().y, waveform, *audioSubsystem);

    if (extras.render && extras.width > 0.0f)
    {
        extras.render(ImRect(ImVec2(left, button_top(firstRow)), ImVec2(left + extras.width, button_top(firstRow) + s_buttonSize)));
        left += extras.width + s_groupSpacing;
    }

    // The right-hand controls run from the right edge; toggles stay on the first row so the waveform view is always reachable.
    float right = render_volume(secondRow.Max.x - s_barPadding, button_top(secondRow), secondRow.GetCenter().y, waveform, *audioSubsystem);
    right       = render_speed(right - s_groupSpacing, secondRow.GetCenter().y, *audioSubsystem);

    const float togglesRight = oneRow ? right - s_groupSpacing : firstRow.Max.x - s_barPadding;
    if (togglesRight - widths.toggles >= left)
    {
        render_overlay_toggles(togglesRight, button_top(firstRow), waveform);
    }
}

auto gluten::audio_routing_controls::get_height(const std::filesystem::path& filePath, float width) -> float
{
    std::shared_ptr<gluten::audio_subsystem> audioSubsystem = get_audio_subsystem();
    const audio_subsystem::file_routing* const routing      = audioSubsystem ? audioSubsystem->get_file_routing(filePath) : nullptr;

    if (!routing || routing->channelCount <= 2)
    {
        return 0.0f;
    }

    if (!s_channelGridOpen)
    {
        return s_barHeight;
    }

    const std::size_t columns = grid_columns(width);
    const std::size_t rows    = (routing->channelCount + columns - 1) / columns;
    return s_barHeight + theme::space04 + static_cast<float>(rows) * s_gridRowHeight;
}

auto gluten::audio_routing_controls::render(const ImRect& area, const std::filesystem::path& filePath) -> void
{
    std::shared_ptr<gluten::audio_subsystem> audioSubsystem = get_audio_subsystem();
    const audio_subsystem::file_routing* const routing      = audioSubsystem ? audioSubsystem->get_file_routing(filePath) : nullptr;

    if (!routing || routing->channelCount <= 2)
    {
        return;
    }

    ImGui::PushID(filePath.string().c_str());

    ImDrawList* const drawList      = ImGui::GetWindowDrawList();
    const sc_channel_layout& layout = audioSubsystem->get_active_layout(*routing);
    const std::size_t channels      = routing->channelCount;
    const bool hasOverride          = audioSubsystem->has_layout_override(channels);

    const ImRect bar(area.Min, ImVec2(area.Max.x, area.Min.y + s_barHeight));
    {
        const std::string preview     = hasOverride ? layout.name : fmt::format("Auto: {}", layout.name);
        const std::string outputLabel = fmt::format("to {}", audioSubsystem->get_output_description());

        const float outputWidth = text_font()->CalcTextSizeA(theme::textSizeHelper, FLT_MAX, 0.0f, outputLabel.c_str()).x;
        const float buttonLeft  = bar.Max.x - audio_transport::s_buttonSize;
        const float outputLeft  = buttonLeft - theme::space08 - outputWidth;

        const float frameHeight = ImGui::GetFrameHeight();
        ImGui::SetCursorScreenPos(ImVec2(bar.Min.x, std::floor(bar.GetCenter().y - frameHeight * 0.5f)));
        ImGui::SetNextItemWidth(std::max(outputLeft - theme::space08 - bar.Min.x, 80.0f));

        if (ImGui::BeginCombo("##Layout", preview.c_str()))
        {
            if (ImGui::Selectable(fmt::format("Auto: {} ({})", routing->detectedLayout.name, routing->detectedReason).c_str(), !hasOverride))
            {
                audioSubsystem->set_layout_override(channels, std::nullopt);
            }

            ImGui::Separator();

            std::array<sc_channel_layout, SC_MAX_CHANNEL_LAYOUT_PRESETS> presets{};
            sc_uint32 presetCount = 0;
            (void)sc_channel_layout_get_presets(static_cast<sc_uint32>(channels), presets.data(), static_cast<sc_uint32>(presets.size()), &presetCount);

            for (const sc_channel_layout& preset : std::span(presets.data(), presetCount))
            {
                if (ImGui::Selectable(preset.name, hasOverride && std::strcmp(preset.name, layout.name) == 0))
                {
                    audioSubsystem->set_layout_override(channels, preset);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SetItemTooltip("How this file's channels are laid out. Picking one applies to every %zu-channel file until you set it back to Auto.", channels);

        draw_text_in(drawList, text_font(), theme::textSizeHelper, ImRect(ImVec2(outputLeft, bar.Min.y), ImVec2(buttonLeft - theme::space08, bar.Max.y)), theme::textHelper, outputLabel);

        const float top = std::floor(bar.GetCenter().y - audio_transport::s_buttonSize * 0.5f);
        if (transport_button("##ChannelGrid", ImRect(buttonLeft, top, bar.Max.x, top + audio_transport::s_buttonSize), ICON_LC_SLIDERS_HORIZONTAL, icon_font(), theme::iconSizeSm, s_channelGridOpen))
        {
            s_channelGridOpen = !s_channelGridOpen;
        }
        ImGui::SetItemTooltip(s_channelGridOpen ? "Hide channel routing" : "Reorder or solo channels");
    }

    if (s_channelGridOpen)
    {
        const ImRect grid(ImVec2(area.Min.x, bar.Max.y + theme::space04), area.Max);
        drawList->AddRectFilled(grid.Min, grid.Max, ImGui::GetColorU32(theme::layer01), theme::radiusSm);

        const std::size_t columns = grid_columns(grid.GetWidth());
        const float cellWidth     = grid.GetWidth() / static_cast<float>(columns);
        const float frameHeight   = ImGui::GetFrameHeight();
        ImFont* const labelFont   = app::get()->get_font(fonts::semibold);

        for (std::size_t slot = 0; slot < channels; ++slot)
        {
            const std::size_t column = slot % columns;
            const std::size_t row    = slot / columns;

            const ImRect cell(ImVec2(grid.Min.x + cellWidth * static_cast<float>(column), grid.Min.y + s_gridRowHeight * static_cast<float>(row)),
                              ImVec2(grid.Min.x + cellWidth * static_cast<float>(column + 1), grid.Min.y + s_gridRowHeight * static_cast<float>(row + 1)));

            ImGui::PushID(static_cast<int>(slot));

            const std::size_t fileChannel = routing->channelOrder[slot];
            const bool soloed             = routing->soloedChannels[fileChannel] != SC_FALSE;
            const std::string slotLabel   = routing->channelLabels[fileChannel];

            const float buttonSize = s_gridRowHeight - 4.0f;
            const float buttonTop  = std::floor(cell.GetCenter().y - buttonSize * 0.5f);
            const ImRect soloRect(ImVec2(cell.Min.x + 4.0f, buttonTop), ImVec2(cell.Min.x + 4.0f + buttonSize, buttonTop + buttonSize));
            if (transport_button("##Solo", soloRect, ICON_LC_HEADPHONES, icon_font(), theme::iconSizeSm, soloed))
            {
                audioSubsystem->set_channel_soloed(filePath, fileChannel, !soloed);
            }
            ImGui::SetItemTooltip("Solo. Ambisonic components play unprocessed so you can hear each one.");

            const ImRect labelRect(ImVec2(soloRect.Max.x + theme::space04, cell.Min.y), ImVec2(soloRect.Max.x + 40.0f, cell.Max.y));
            draw_text_in(drawList, labelFont, theme::textSizeHelper, labelRect, theme::textPrimary, slotLabel);

            ImGui::SetCursorScreenPos(ImVec2(labelRect.Max.x, std::floor(cell.GetCenter().y - frameHeight * 0.5f)));
            ImGui::SetNextItemWidth(cell.Max.x - labelRect.Max.x - theme::space08);

            if (ImGui::BeginCombo("##Source", fmt::format("ch {}", fileChannel + 1).c_str()))
            {
                for (std::size_t candidate = 0; candidate < channels; ++candidate)
                {
                    if (ImGui::Selectable(fmt::format("ch {}", candidate + 1).c_str(), candidate == fileChannel))
                    {
                        audioSubsystem->set_channel_source(filePath, slot, candidate);
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SetItemTooltip("Which channel of the file plays as %s", slotLabel.c_str());

            ImGui::PopID();
        }
    }

    ImGui::PopID();
}
