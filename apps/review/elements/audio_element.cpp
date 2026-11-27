#include "audio_element.h"

#include "IconsLucide.h"
#include "gluten/subsystems/audio_subsystem.h"
#include "implot.h"
#include "implot_internal.h"
#include "managers/workspace_manager.h"

namespace
{
    constexpr double s_adjacentCommentGap = 0.05;
    constexpr float s_commentNavWidth     = 28.0f;

    auto comment_color() -> ImVec4 { return gluten::theme::hex_to_imgui_imvec4(0x08bdba); }

    auto comment_button_label(double position) -> std::string
    {
        return fmt::format(ICON_LC_MESSAGE_SQUARE_PLUS "  Comment at {}", gluten::format_timecode(position, false));
    }

    auto comment_button_width(const std::string& label) -> float
    {
        ImFont* const font = gluten::app::get()->get_font(gluten::fonts::regular_lucide_icons);
        return std::ceil(font->CalcTextSizeA(gluten::theme::textSizeHelper, FLT_MAX, 0.0f, label.c_str()).x) + gluten::theme::space08 * 2.0f + 4.0f;
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
    const float width       = layoutRect.GetWidth();

    bool createdComment                   = false;
    const gluten::transport_extras extras = make_comment_controls(createdComment);

    const float transportHeight = gluten::audio_transport::get_height(width, extras.width);
    const float routingHeight   = gluten::audio_routing_controls::get_height(m_filePath, width);

    const ImRect transportRect(layoutRect.Min, ImVec2(layoutRect.Max.x, layoutRect.Min.y + transportHeight));
    float waveformTop = transportRect.Max.y + gluten::theme::space04;

    const ImRect routingRect(ImVec2(layoutRect.Min.x, waveformTop), ImVec2(layoutRect.Max.x, waveformTop + routingHeight));
    if (routingHeight > 0.0f)
    {
        waveformTop = routingRect.Max.y + gluten::theme::space04;
    }

    const ImRect waveformRect(ImVec2(layoutRect.Min.x, waveformTop), ImVec2(layoutRect.Max.x, std::max(waveformTop, layoutRect.Max.y)));

    m_layout.render_spacer_pixels(0.0f, waveformTop - layoutRect.Min.y - gluten::theme::space04);

    m_waveform.render(waveformRect);
    m_layout.render_spacer_pixels(0.0f, waveformRect.GetHeight());

    gluten::audio_transport::render(transportRect, m_waveform, extras);

    if (routingHeight > 0.0f)
    {
        gluten::audio_routing_controls::render(routingRect, m_filePath);
    }

    if (renderInfo.isVisible)
    {
        handle_keyboard_controls(m_waveform.get_element_rect());
    }

    return createdComment;
}

auto audio_element::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
{
    const float transportHeight = gluten::audio_transport::get_height(parentSize.x, make_comment_controls_width());
    const float routingHeight   = gluten::audio_routing_controls::get_height(m_filePath, parentSize.x);
    const float routingSpace    = routingHeight > 0.0f ? routingHeight + gluten::theme::space04 : 0.0f;

    return ImVec2(parentSize.x, m_waveform.get_element_content_size(parentSize).y + transportHeight + gluten::theme::space04 + routingSpace);
}

auto audio_element::make_comment_controls_width() const -> float
{
    return s_commentNavWidth * 2.0f + comment_button_width(comment_button_label(m_filePosition)) - 2.0f;
}

auto audio_element::make_comment_controls(bool& createdComment) -> gluten::transport_extras
{
    gluten::transport_extras extras;
    extras.width  = make_comment_controls_width();
    extras.render = [this, &createdComment](const ImRect& area)
    {
        ImFont* const lucideFont = gluten::app::get()->get_font(gluten::fonts::regular_lucide_icons);
        const std::string label  = comment_button_label(m_filePosition);
        const float labelWidth   = comment_button_width(label);

        float x                = area.Min.x;
        const auto next_rect   = [&x, &area](float width) { return ImRect(ImVec2(x, area.Min.y), ImVec2(x + width, area.Max.y)); };

        if (gluten::transport_button("##PrevComment", next_rect(s_commentNavWidth), ICON_LC_CHEVRON_LEFT, lucideFont, gluten::theme::iconSizeMd, false, ImDrawFlags_RoundCornersLeft))
        {
            seek_to_adjacent_comment(false);
        }
        ImGui::SetItemTooltip("Previous comment");
        x += s_commentNavWidth - 1.0f;

        if (gluten::transport_button("##AddComment", next_rect(labelWidth), label.c_str(), lucideFont, gluten::theme::textSizeHelper, false, ImDrawFlags_RoundCornersNone))
        {
            createdComment = true;
            pause_file();
        }
        ImGui::SetItemTooltip("Add a comment at the playhead");
        x += labelWidth - 1.0f;

        if (gluten::transport_button("##NextComment", next_rect(s_commentNavWidth), ICON_LC_CHEVRON_RIGHT, lucideFont, gluten::theme::iconSizeMd, false, ImDrawFlags_RoundCornersRight))
        {
            seek_to_adjacent_comment(true);
        }
        ImGui::SetItemTooltip("Next comment");
    };
    return extras;
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
