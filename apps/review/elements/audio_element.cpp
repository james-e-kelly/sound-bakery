#include "audio_element.h"

#include "elements/comment_markers.h"
#include "gluten/subsystems/audio_subsystem.h"
#include "implot.h"
#include "implot_internal.h"

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

    if (createdComment)
    {
        pause_file();
    }

    if (renderInfo.isVisible)
    {
        handle_keyboard_controls(m_waveform.get_element_rect());
    }

    return createdComment;
}

auto audio_element::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
{
    const float transportHeight = gluten::audio_transport::get_height(parentSize.x, comment_markers::controls_width(comment_position_label()));
    const float routingHeight   = gluten::audio_routing_controls::get_height(m_filePath, parentSize.x);
    const float routingSpace    = routingHeight > 0.0f ? routingHeight + gluten::theme::space04 : 0.0f;

    return ImVec2(parentSize.x, m_waveform.get_element_content_size(parentSize).y + transportHeight + gluten::theme::space04 + routingSpace);
}

auto audio_element::comment_position_label() const -> std::string
{
    return gluten::format_timecode(m_filePosition, false);
}

auto audio_element::make_comment_controls(bool& createdComment) -> gluten::transport_extras
{
    return comment_markers::make_controls(m_fileId, comment_position_label(), m_filePosition, [this](double time)
                                          { seek_to_position(time); }, createdComment);
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

auto audio_element::waveform_element::render_waveform_overlay(const ImRect& markerLane, double plotTimeWidth) -> void
{
    const std::vector<const comment_data*> fileComments = comment_markers::get_file_comments(m_fileId);
    if (fileComments.empty())
    {
        return;
    }

    const ImVec4 markerColor               = comment_markers::color();
    const ImU32 lineColor                  = ImGui::GetColorU32(gluten::theme::adjust_alpha(markerColor, 0.7f));
    const ImU32 rangeColor                 = ImGui::GetColorU32(gluten::theme::adjust_alpha(markerColor, 0.10f));
    constexpr double s_compactZoomFraction = 0.5;  //< Show initials only while more than this much of the file is visible

    const bool compact = m_fileDuration > 0.0 && plotTimeWidth / m_fileDuration > s_compactZoomFraction;

    ImDrawList* const plotDrawList = ImPlot::GetPlotDrawList();
    const ImVec2 plotMin           = ImPlot::GetPlotPos();
    const ImVec2 plotMax           = ImVec2(plotMin.x + ImPlot::GetPlotSize().x, plotMin.y + ImPlot::GetPlotSize().y);

    const std::function<float(double)> time_to_x = [](double time)
    { return ImPlot::PlotToPixels(time, 0.0).x; };
    const auto pixel_x = [&time_to_x](double time)
    { return std::floor(time_to_x(time)) + 0.5f; };

    ImPlot::PushPlotClipRect();
    for (const comment_data* comment : fileComments)
    {
        const float x = pixel_x(comment->m_timeStart);
        if (comment->m_timeEnd > comment->m_timeStart)
        {
            plotDrawList->AddRectFilled(ImVec2(x, plotMin.y), ImVec2(pixel_x(comment->m_timeEnd), plotMax.y), rangeColor);
        }
        plotDrawList->AddLine(ImVec2(x, plotMin.y), ImVec2(x, plotMax.y), lineColor);
    }
    ImPlot::PopPlotClipRect();

    if (const std::optional<double> seekTime = comment_markers::render_tags(markerLane, fileComments, time_to_x, compact, [](double time)
                                                                            { return gluten::format_timecode(time); }))
    {
        seek(*seekTime);
    }
}
