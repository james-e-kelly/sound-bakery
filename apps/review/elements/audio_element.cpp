#include "audio_element.h"

#include "IconsFontaudio.h"
#include "IconsLucide.h"
#include "gluten/subsystems/audio_subsystem.h"
#include "implot.h"
#include "implot_internal.h"
#include "managers/workspace_manager.h"

namespace
{
    constexpr float s_barPadding          = 6.0f;
    constexpr float s_buttonSize          = 28.0f;
    constexpr float s_groupSpacing        = 8.0f;
    constexpr float s_faderWidth          = 120.0f;
    constexpr float s_meterWidth          = 48.0f;
    constexpr float s_volumeMinDb         = -60.0f;  //< Bottom of the fader, treated as silence
    constexpr float s_volumeMaxDb         = 12.0f;
    constexpr float s_wheelStepDb         = 0.5f;
    constexpr float s_meterMinDb          = -60.0f;
    constexpr float s_meterMaxDb          = 0.0f;
    constexpr float s_defaultVolumeDb     = -12.0f;
    constexpr float s_meterFallDbPerSec   = 24.0f;
    constexpr float s_meterYellowDb       = -12.0f;
    constexpr float s_meterRedDb          = -3.0f;
    constexpr double s_adjacentCommentGap = 0.05;

    auto comment_color() -> ImVec4 { return gluten::theme::hex_to_imgui_imvec4(0x08bdba); }

    auto linear_to_db(float linear) -> float { return linear > 0.0f ? 20.0f * std::log10(linear) : -std::numeric_limits<float>::infinity(); }
    auto db_to_linear(float db) -> float { return db <= s_volumeMinDb ? 0.0f : std::pow(10.0f, db / 20.0f); }

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

    auto get_audio_subsystem() -> std::shared_ptr<gluten::audio_subsystem>
    {
        return gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>();
    }

    /**
     * @brief A square-cornered transport button. Neighbouring buttons overlap by a pixel and only round
     * their outer corners, so a run of them reads as one segmented control.
     */
    auto segment_button(const char* id, const ImRect& rect, ImDrawFlags corners, bool active, const char* label, ImFont* font, float fontSize, const ImVec4& activeFill = gluten::theme::interactive, const ImVec4& activeText = gluten::theme::textOnColor) -> bool
    {
        ImGui::SetCursorScreenPos(rect.Min);
        const bool pressed = ImGui::InvisibleButton(id, rect.GetSize());
        const bool hovered = ImGui::IsItemHovered();
        const bool held    = ImGui::IsItemActive();

        ImVec4 background = ImLerp(gluten::theme::layer02, gluten::theme::layer03, 0.35f);
        if (active)
        {
            background = activeFill;
        }
        else if (held)
        {
            background = gluten::theme::layerActive02;
        }
        else if (hovered)
        {
            background = gluten::theme::layerHover02;
        }

        ImDrawList* const drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(rect.Min, rect.Max, ImGui::GetColorU32(background), gluten::theme::radiusSm, corners);
        drawList->AddRect(rect.Min, rect.Max, ImGui::GetColorU32(active ? activeFill : gluten::theme::borderStrong01), gluten::theme::radiusSm, corners);

        const ImVec2 labelSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, label);
        drawList->AddText(font, fontSize, ImVec2(std::floor(rect.GetCenter().x - labelSize.x * 0.5f), std::floor(rect.GetCenter().y - labelSize.y * 0.5f)),
                          ImGui::GetColorU32(active ? activeText : gluten::theme::textPrimary), label);

        return pressed;
    }

    auto text_button_width(ImFont* font, const char* label) -> float
    {
        return std::ceil(font->CalcTextSizeA(gluten::theme::textSizeHelper, FLT_MAX, 0.0f, label).x) + gluten::theme::space08 * 2.0f + 4.0f;
    }

    auto initials_of(const std::string& displayName) -> std::string
    {
        std::string initials;
        bool atWordStart = true;
        for (const char& character : displayName)
        {
            if (std::isspace(static_cast<unsigned char>(character)))
            {
                atWordStart = true;
            }
            else if (atWordStart && initials.size() < 2)
            {
                initials += static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
                atWordStart = false;
            }
            else
            {
                atWordStart = false;
            }
        }
        return initials.empty() ? std::string("?") : initials;
    }
}  // namespace

audio_element::audio_element(const std::filesystem::path& filePath, const int64_t fileId)
    : gluten::file_element(gluten::anchor_preset::stretch_full, filePath), m_fileId(fileId), m_waveform(filePath, fileId)
{
    m_layout.set_layout_spacing(gluten::theme::space04);
    m_layout.set_element_padding(ImVec2(gluten::theme::space08, gluten::theme::space08));
}

auto audio_element::render_element(const gluten::element_render_info& renderInfo) -> bool
{
    ZoneScoped;

    m_layout.render(renderInfo.elementBox);

    // m_waveform has its own get_element_content_size() opinion (based on width), and layout helpers always take
    // max(requested, desired), so it is placed with an exact box instead and spacers keep m_layout's cursor in sync.
    const ImRect layoutRect = m_layout.get_element_rect();
    const ImRect transportRect(layoutRect.Min, ImVec2(layoutRect.Max.x, layoutRect.Min.y + s_controlHeight));
    const float waveformTop = transportRect.Max.y + gluten::theme::space04;
    const ImRect waveformRect(ImVec2(layoutRect.Min.x, waveformTop), ImVec2(layoutRect.Max.x, std::max(waveformTop, layoutRect.Max.y)));

    m_layout.render_spacer_pixels(0.0f, s_controlHeight);

    m_waveform.render(waveformRect);
    m_layout.render_spacer_pixels(0.0f, waveformRect.GetHeight());

    const bool createdComment = render_transport(transportRect);

    if (renderInfo.isVisible)
    {
        handle_keyboard_controls(m_waveform.get_element_rect());
    }

    return createdComment;
}

auto audio_element::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
{
    return ImVec2(parentSize.x, m_waveform.get_element_content_size(parentSize).y + s_controlHeight + gluten::theme::space04);
}

auto audio_element::render_transport(const ImRect& bar) -> bool
{
    ZoneScoped;

    std::shared_ptr<gluten::audio_subsystem> audioSubsystem = get_audio_subsystem();

    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    ImFont* const textFont     = gluten::app::get()->get_font(gluten::fonts::regular);
    ImFont* const audioFont    = gluten::app::get()->get_font(gluten::fonts::regular_audio_icons);
    ImFont* const lucideFont   = gluten::app::get()->get_font(gluten::fonts::regular_lucide_icons);

    drawList->AddRectFilled(bar.Min, bar.Max, ImGui::GetColorU32(gluten::theme::layer02), gluten::theme::radiusSm);

    const float top     = std::floor(bar.Min.y + (bar.GetHeight() - s_buttonSize) * 0.5f);
    const float centerY = bar.GetCenter().y;
    float x             = bar.Min.x + s_barPadding;

    const auto next_rect = [&](float width) -> ImRect
    {
        return ImRect(ImVec2(x, top), ImVec2(x + width, top + s_buttonSize));
    };

    // Stop / play
    if (segment_button("##Stop", next_rect(30.0f), ImDrawFlags_RoundCornersLeft, false, ICON_FAD_STOP, audioFont, gluten::theme::iconSizeMd))
    {
        pause_file();
        seek_to_position(0.0);
    }
    ImGui::SetItemTooltip("Stop");
    x += 29.0f;

    const bool playing = get_is_playing();
    if (segment_button("##PlayPause", next_rect(30.0f), ImDrawFlags_RoundCornersRight, playing, playing ? ICON_FAD_PAUSE : ICON_FAD_PLAY, audioFont, gluten::theme::iconSizeMd))
    {
        playing ? pause_file() : play_file();
    }
    ImGui::SetItemTooltip(playing ? "Pause (Space)" : "Play (Space)");
    x += 30.0f + s_groupSpacing;

    // Timecode
    {
        const std::string position     = gluten::format_timecode(m_filePosition);
        const std::size_t decimalPoint = position.find('.');
        const std::string wholePart    = position.substr(0, decimalPoint);
        const std::string fractionPart = decimalPoint == std::string::npos ? std::string() : position.substr(decimalPoint);
        const std::string duration     = fmt::format("/ {}", gluten::format_timecode(m_fileDuration));

        const float mainSize = gluten::theme::textSizeSubtitle;
        const float subSize  = gluten::theme::textSizeHelper;

        const float wholeWidth    = gluten::calc_tabular_text_width(textFont, mainSize, wholePart);
        const float fractionWidth = gluten::calc_tabular_text_width(textFont, mainSize, fractionPart);
        const float durationWidth = gluten::calc_tabular_text_width(textFont, subSize, duration);
        const float boxWidth      = std::ceil(wholeWidth + fractionWidth + durationWidth + 6.0f + gluten::theme::space08 * 2.0f);

        const ImRect box = next_rect(boxWidth);
        drawList->AddRectFilled(box.Min, box.Max, ImGui::GetColorU32(gluten::theme::background), 2.0f);
        drawList->AddRect(box.Min, box.Max, ImGui::GetColorU32(gluten::theme::borderSubtle01), 2.0f);

        const float mainY = std::floor(box.GetCenter().y - mainSize * 0.5f - 1.0f);
        const float subY  = mainY + (mainSize - subSize) - 1.0f;
        float textX       = box.Min.x + gluten::theme::space08;
        textX += gluten::draw_tabular_text(drawList, textFont, mainSize, ImVec2(textX, mainY), ImGui::GetColorU32(gluten::theme::textPrimary), wholePart);
        textX += gluten::draw_tabular_text(drawList, textFont, mainSize, ImVec2(textX, mainY), ImGui::GetColorU32(gluten::theme::textSecondary), fractionPart);
        gluten::draw_tabular_text(drawList, textFont, subSize, ImVec2(textX + 6.0f, subY), ImGui::GetColorU32(gluten::theme::textHelper), duration);

        x += boxWidth + s_groupSpacing;
    }

    // Comments
    if (segment_button("##PrevComment", next_rect(28.0f), ImDrawFlags_RoundCornersLeft, false, ICON_LC_CHEVRON_LEFT, lucideFont, gluten::theme::iconSizeMd))
    {
        seek_to_adjacent_comment(false);
    }
    ImGui::SetItemTooltip("Previous comment");
    x += 27.0f;

    const std::string commentLabel = fmt::format(ICON_LC_MESSAGE_SQUARE_PLUS "  Comment at {}", gluten::format_timecode(m_filePosition, false));
    const float commentWidth       = text_button_width(lucideFont, commentLabel.c_str());
    const bool newComment          = segment_button("##AddComment", next_rect(commentWidth), ImDrawFlags_RoundCornersNone, false, commentLabel.c_str(), lucideFont, gluten::theme::textSizeHelper);
    ImGui::SetItemTooltip("Add a comment at the playhead");
    x += commentWidth - 1.0f;

    if (newComment)
    {
        pause_file();
    }

    if (segment_button("##NextComment", next_rect(28.0f), ImDrawFlags_RoundCornersRight, false, ICON_LC_CHEVRON_RIGHT, lucideFont, gluten::theme::iconSizeMd))
    {
        seek_to_adjacent_comment(true);
    }
    ImGui::SetItemTooltip("Next comment");
    x += 28.0f;

    const float leftEdge = x + s_groupSpacing;

    // Volume and meter, laid out from the right edge
    float right = bar.Max.x - s_barPadding;

    static float s_faderDb = s_defaultVolumeDb;
    static bool s_muted    = false;

    const float previewVolume = s_muted ? 0.0f : db_to_linear(s_faderDb);
    if (audioSubsystem && audioSubsystem->get_preview_volume() != previewVolume)
    {
        audioSubsystem->set_preview_volume(previewVolume);
    }

    {
        static std::unordered_map<std::filesystem::path, std::vector<float>> s_meterLevelsDb;
        std::vector<float>& meterLevelsDb = s_meterLevelsDb[m_filePath];

        const std::vector<float> peaks = playing ? m_waveform.get_channel_peaks_at(m_filePosition) : std::vector<float>();
        const std::size_t rows         = std::clamp<std::size_t>(m_waveform.get_channel_count(), 1, 2);
        meterLevelsDb.resize(rows, s_meterMinDb);

        const float fall = s_meterFallDbPerSec * ImGui::GetIO().DeltaTime;
        for (std::size_t row = 0; row < rows; ++row)
        {
            const float peak    = peaks.empty() ? 0.0f : peaks[std::min(row, peaks.size() - 1)];
            const float levelDb = std::max(linear_to_db(peak * previewVolume), s_meterMinDb);
            meterLevelsDb[row]  = std::max(levelDb, meterLevelsDb[row] - fall);
        }

        const float rowHeight = 4.0f;
        const float rowGap    = 2.0f;
        const float meterTop  = std::floor(centerY - (rows * rowHeight + (rows - 1) * rowGap) * 0.5f);
        const float meterLeft = right - s_meterWidth;

        const auto db_to_x = [&](float db)
        { return meterLeft + (db - s_meterMinDb) / (s_meterMaxDb - s_meterMinDb) * s_meterWidth; };

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
        ImGui::SetItemTooltip("Output level after the preview volume");

        right = meterLeft - s_groupSpacing;
    }

    {
        const float readoutWidth  = gluten::calc_tabular_text_width(textFont, gluten::theme::textSizeHelper, "+00.0 dB");
        const std::string readout = s_faderDb <= s_volumeMinDb ? std::string("-inf dB") : fmt::format("{:+.1f} dB", s_faderDb);
        gluten::draw_tabular_text(drawList, textFont, gluten::theme::textSizeHelper, ImVec2(right - readoutWidth, std::floor(centerY - gluten::theme::textSizeHelper * 0.5f - 1.0f)),
                                  ImGui::GetColorU32(s_muted ? gluten::theme::textHelper : gluten::theme::textSecondary), readout);
        right -= readoutWidth + 6.0f;
    }

    {
        const ImRect fader(ImVec2(right - s_faderWidth, centerY - 8.0f), ImVec2(right, centerY + 8.0f));
        ImGui::SetCursorScreenPos(fader.Min);
        ImGui::InvisibleButton("##Volume", fader.GetSize());
        const bool faderHovered = ImGui::IsItemHovered();
        const bool faderActive  = ImGui::IsItemActive();

        if (faderActive)
        {
            s_faderDb = fader_position_to_db(std::clamp((ImGui::GetMousePos().x - fader.Min.x) / fader.GetWidth(), 0.0f, 1.0f));
        }
        if (faderHovered && ImGui::GetIO().MouseWheel != 0.0f)
        {
            s_faderDb = std::clamp(std::round((s_faderDb + ImGui::GetIO().MouseWheel * s_wheelStepDb) / s_wheelStepDb) * s_wheelStepDb, s_volumeMinDb, s_volumeMaxDb);
        }
        if (faderHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            s_faderDb = s_defaultVolumeDb;
        }
        ImGui::SetItemTooltip("Preview volume. Scroll to nudge, double-click to reset.");

        const float handleX = std::floor(fader.Min.x + db_to_fader_position(s_faderDb) * fader.GetWidth());
        const float unityX  = std::floor(fader.Min.x + db_to_fader_position(0.0f) * fader.GetWidth()) + 0.5f;
        const ImRect track(ImVec2(fader.Min.x, centerY - 2.0f), ImVec2(fader.Max.x, centerY + 2.0f));

        drawList->AddRectFilled(track.Min, track.Max, ImGui::GetColorU32(gluten::theme::background));
        drawList->AddRectFilled(track.Min, ImVec2(handleX, track.Max.y), ImGui::GetColorU32(gluten::theme::textHelper));
        drawList->AddRect(track.Min, track.Max, ImGui::GetColorU32(gluten::theme::borderSubtle01));
        drawList->AddLine(ImVec2(unityX, centerY - 6.0f), ImVec2(unityX, centerY - 3.0f), ImGui::GetColorU32(gluten::theme::textHelper));
        drawList->AddLine(ImVec2(unityX, centerY + 3.0f), ImVec2(unityX, centerY + 6.0f), ImGui::GetColorU32(gluten::theme::textHelper));
        drawList->AddRectFilled(ImVec2(handleX - 3.0f, centerY - 7.0f), ImVec2(handleX + 4.0f, centerY + 7.0f),
                                ImGui::GetColorU32(faderHovered || faderActive ? gluten::theme::textPrimary : gluten::theme::textSecondary), 1.0f);
        drawList->AddRect(ImVec2(handleX - 3.0f, centerY - 7.0f), ImVec2(handleX + 4.0f, centerY + 7.0f), ImGui::GetColorU32(gluten::theme::background), 1.0f);

        right = fader.Min.x - 6.0f;
    }

    {
        const ImRect muteRect(ImVec2(right - s_buttonSize, top), ImVec2(right, top + s_buttonSize));
        if (segment_button("##Mute", muteRect, ImDrawFlags_RoundCornersAll, s_muted, "M", gluten::app::get()->get_font(gluten::fonts::semibold), gluten::theme::textSizeBody, gluten::theme::supportWarning, gluten::theme::background))
        {
            s_muted = !s_muted;
        }
        ImGui::SetItemTooltip(s_muted ? "Unmute" : "Mute");

        right = muteRect.Min.x - s_groupSpacing;
    }

    // Overlay toggles, dropped first when space runs out
    {
        const char* loudnessLabel = "Loudness";
        const char* bandsLabel    = "Bands";
        const float loudnessWidth = text_button_width(textFont, loudnessLabel);
        const float bandsWidth    = text_button_width(textFont, bandsLabel);

        if (right - loudnessWidth - bandsWidth > leftEdge)
        {
            const gluten::waveform_overlay overlay = m_waveform.get_overlay();
            const auto toggle                      = [this, overlay](gluten::waveform_overlay target)
            {
                m_waveform.set_overlay(overlay == target ? gluten::waveform_overlay::none : target);
            };

            const ImRect bandsRect(ImVec2(right - bandsWidth, top), ImVec2(right, top + s_buttonSize));
            const ImRect loudnessRect(ImVec2(bandsRect.Min.x - loudnessWidth + 1.0f, top), ImVec2(bandsRect.Min.x + 1.0f, top + s_buttonSize));

            if (segment_button("##LoudnessOverlay", loudnessRect, ImDrawFlags_RoundCornersLeft, overlay == gluten::waveform_overlay::loudness, loudnessLabel, textFont, gluten::theme::textSizeHelper))
            {
                toggle(gluten::waveform_overlay::loudness);
            }
            ImGui::SetItemTooltip("Short-term and integrated loudness");

            if (segment_button("##BandsOverlay", bandsRect, ImDrawFlags_RoundCornersRight, overlay == gluten::waveform_overlay::bands, bandsLabel, textFont, gluten::theme::textSizeHelper))
            {
                toggle(gluten::waveform_overlay::bands);
            }
            ImGui::SetItemTooltip("Low, mid and high band levels");
        }
    }

    return newComment;
}

auto audio_element::seek_to_adjacent_comment(bool forwards) -> void
{
    const std::vector<double> commentTimes = m_waveform.get_comment_times();

    if (forwards)
    {
        const auto next = std::find_if(commentTimes.begin(), commentTimes.end(), [this](double time)
                                       { return time > m_filePosition + s_adjacentCommentGap; });
        if (next != commentTimes.end())
        {
            seek_to_position(*next);
        }
    }
    else
    {
        const auto previous = std::find_if(commentTimes.rbegin(), commentTimes.rend(), [this](double time)
                                           { return time < m_filePosition - s_adjacentCommentGap; });
        if (previous != commentTimes.rend())
        {
            seek_to_position(*previous);
        }
    }
}

auto audio_element::get_file_play_position() const -> double
{
    return m_waveform.get_position();
}

auto audio_element::get_file_duration() const -> double
{
    return m_waveform.get_duration();
}

auto audio_element::play_file() -> void
{
    m_waveform.play();
}

auto audio_element::pause_file() -> void
{
    m_waveform.pause();
}

auto audio_element::prev_frame() -> void
{
    m_waveform.seek(std::max(m_waveform.get_position() - 1.0, 0.0));
}

auto audio_element::next_frame() -> void
{
    m_waveform.seek(std::min(m_waveform.get_position() + 1.0, m_waveform.get_duration()));
}

auto audio_element::get_is_playing() -> bool
{
    return m_waveform.is_playing();
}

auto audio_element::seek_to_position(double position) -> void
{
    m_waveform.seek(position);
}

audio_element::waveform_element::waveform_element(const std::filesystem::path& filePath, int64_t fileId)
    : gluten::audio_element(filePath), m_fileId(fileId)
{
}

auto audio_element::waveform_element::get_comment_times() const -> std::vector<double>
{
    std::vector<double> times;

    if (std::shared_ptr<workspace_manager> workspaceManager = gluten::app::get()->get_manager_by_class<workspace_manager>())
    {
        const auto& comments = workspaceManager->get_all_comments_for_review(workspaceManager->get_selected_review().m_reviewId);

        if (comments.has_data())
        {
            for (const auto& comment : comments.m_cache)
            {
                if (comment.m_fileId == m_fileId && comment.m_timeStart >= 0.0)
                {
                    times.push_back(comment.m_timeStart);
                }
            }
        }
    }

    std::sort(times.begin(), times.end());
    return times;
}

auto audio_element::waveform_element::render_waveform_overlay(const ImRect& markerLane, double plotTimeWidth) -> void
{
    std::shared_ptr<workspace_manager> workspaceManager = gluten::app::get()->get_manager_by_class<workspace_manager>();
    if (!workspaceManager)
    {
        return;
    }

    const auto& comments = workspaceManager->get_all_comments_for_review(workspaceManager->get_selected_review().m_reviewId);
    if (!comments.has_data())
    {
        return;
    }

    std::vector<const comment_data*> fileComments;
    for (const auto& comment : comments.m_cache)
    {
        if (comment.m_fileId == m_fileId && comment.m_timeStart >= 0.0)
        {
            fileComments.push_back(&comment);
        }
    }
    std::sort(fileComments.begin(), fileComments.end(), [](const comment_data* a, const comment_data* b)
              { return a->m_timeStart < b->m_timeStart; });

    const ImVec4 markerColor               = comment_color();
    const ImU32 tagColor                   = ImGui::GetColorU32(markerColor);
    const ImU32 tagTextColor               = ImGui::GetColorU32(gluten::theme::background);
    const ImU32 lineColor                  = ImGui::GetColorU32(gluten::theme::adjust_alpha(markerColor, 0.7f));
    const ImU32 rangeColor                 = ImGui::GetColorU32(gluten::theme::adjust_alpha(markerColor, 0.10f));
    const float labelSize                  = gluten::theme::textSizeCaption;
    constexpr float s_maxTagWidth          = 220.0f;
    constexpr float s_flagWidth            = 5.0f;
    constexpr double s_compactZoomFraction = 0.5;  //< Show initials only while more than this much of the file is visible

    const bool compact = m_fileDuration > 0.0 && plotTimeWidth / m_fileDuration > s_compactZoomFraction;

    ImFont* const font             = gluten::app::get()->get_font(gluten::fonts::semibold);
    ImDrawList* const plotDrawList = ImPlot::GetPlotDrawList();
    ImDrawList* const drawList     = ImGui::GetWindowDrawList();
    const ImVec2 plotMin           = ImPlot::GetPlotPos();
    const ImVec2 plotMax           = ImVec2(plotMin.x + ImPlot::GetPlotSize().x, plotMin.y + ImPlot::GetPlotSize().y);

    const auto time_to_x = [](double time)
    { return std::floor(ImPlot::PlotToPixels(time, 0.0).x) + 0.5f; };

    ImPlot::PushPlotClipRect();
    for (const comment_data* comment : fileComments)
    {
        const float x = time_to_x(comment->m_timeStart);
        if (comment->m_timeEnd > comment->m_timeStart)
        {
            plotDrawList->AddRectFilled(ImVec2(x, plotMin.y), ImVec2(time_to_x(comment->m_timeEnd), plotMax.y), rangeColor);
        }
        plotDrawList->AddLine(ImVec2(x, plotMin.y), ImVec2(x, plotMax.y), lineColor);
    }
    ImPlot::PopPlotClipRect();

    drawList->PushClipRect(markerLane.Min, markerLane.Max, true);
    for (std::size_t index = 0; index < fileComments.size(); ++index)
    {
        const comment_data* comment = fileComments[index];
        const float x               = time_to_x(comment->m_timeStart);

        if (comment->m_timeEnd > comment->m_timeStart)
        {
            drawList->AddRectFilled(ImVec2(x, markerLane.Max.y - 3.0f), ImVec2(time_to_x(comment->m_timeEnd), markerLane.Max.y - 1.0f), tagColor);
        }
        drawList->AddLine(ImVec2(x, markerLane.Min.y + 3.0f), ImVec2(x, markerLane.Max.y), tagColor);

        const std::string displayName = workspaceManager->get_user(comment->m_userId).m_displayName;
        const std::string initials    = initials_of(displayName);
        const std::string firstLine   = comment->m_comment.substr(0, comment->m_comment.find('\n'));
        const std::string label       = compact ? initials : fmt::format("{}  {}", initials, firstLine);

        // Tags shrink to make room for the next marker: full preview, then initials, then a bare flag.
        const float nextX         = index + 1 < fileComments.size() ? time_to_x(fileComments[index + 1]->m_timeStart) - 2.0f : markerLane.Max.x;
        const float initialsWidth = font->CalcTextSizeA(labelSize, FLT_MAX, 0.0f, initials.c_str()).x + 10.0f;
        const float labelWidth    = font->CalcTextSizeA(labelSize, FLT_MAX, 0.0f, label.c_str()).x + 10.0f;
        const float roomToNext    = nextX - x;
        const float tagWidth      = roomToNext >= initialsWidth ? std::max(std::min({labelWidth, roomToNext, s_maxTagWidth}), initialsWidth) : std::max(roomToNext, s_flagWidth);

        const ImRect tag(ImVec2(x, markerLane.Min.y + 3.0f), ImVec2(x + tagWidth, markerLane.Max.y - 5.0f));
        drawList->AddRectFilled(tag.Min, tag.Max, tagColor, 2.0f, ImDrawFlags_RoundCornersRight);

        drawList->PushClipRect(tag.Min, ImVec2(tag.Max.x - 3.0f, tag.Max.y), true);
        drawList->AddText(font, labelSize, ImVec2(tag.Min.x + 5.0f, std::floor(tag.GetCenter().y - labelSize * 0.5f - 1.0f)), tagTextColor, label.c_str());
        drawList->PopClipRect();

        if (ImGui::IsMouseHoveringRect(tag.Min, tag.Max))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImGui::SetTooltip("%s at %s\n%s", displayName.c_str(), gluten::format_timecode(comment->m_timeStart).c_str(), comment->m_comment.c_str());

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                seek(comment->m_timeStart);
            }
        }
    }
    drawList->PopClipRect();
}
