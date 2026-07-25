#include "audio_element.h"

#include "implot.h"
#include "implot_internal.h"
#include "managers/workspace_manager.h"

audio_element::audio_element(const std::filesystem::path& filePath, const int64_t fileId)
    : gluten::file_element(gluten::anchor_preset::stretch_full, filePath), m_fileId(fileId), m_waveform(filePath, fileId)
{
    m_layout.set_layout_spacing(gluten::theme::padding);
    m_layout.set_element_padding(gluten::theme::paddingVec);

    m_playButton.set_element_active_color(gluten::theme::layerActive01);
    m_pauseButton.set_element_active_color(gluten::theme::layerActive01);
    m_previousFrameButton.set_element_active_color(gluten::theme::layerActive01);
    m_nextFrameButton.set_element_active_color(gluten::theme::layerActive01);
    m_addCommentButton.set_element_active_color(gluten::theme::layerActive01);

    m_playButton.set_element_hover_color(gluten::theme::layerHover01);
    m_pauseButton.set_element_hover_color(gluten::theme::layerHover01);
    m_previousFrameButton.set_element_hover_color(gluten::theme::layerHover01);
    m_nextFrameButton.set_element_hover_color(gluten::theme::layerHover01);
    m_addCommentButton.set_element_hover_color(gluten::theme::layerHover01);

    m_filePositionText.set_element_anchor_preset(gluten::anchor_preset::center_middle);
    m_fileDurationText.set_element_anchor_preset(gluten::anchor_preset::center_middle);

    m_filePositionText.set_text_alignment(gluten::text_alignment::center);
    m_fileDurationText.set_text_alignment(gluten::text_alignment::center);

    m_filePositionText.set_text("00:00");
    m_fileDurationText.set_text("00:00");

    m_controlButtonsLayout.set_element_alignment(ImVec2(-0.5f, 0.0f));
    m_controlButtonsLayout.get_element_anchor().minOffset.x -= s_controlButtonsWidth + (s_buttonWidth * 3.0f);
    m_controlButtonsLayout.get_element_anchor().maxOffset.x += s_controlButtonsWidth + (s_buttonWidth * 3.0f);
    m_controlButtonsLayout.set_element_rounding(gluten::theme::rounding);
    m_controlButtonsLayout.set_element_background_color(gluten::theme::layer02);
    m_controlButtonsLayout.set_element_max_size(ImVec2(0.0f, s_controlButtonsWidth));
}

auto audio_element::render_element(const gluten::element_render_info& renderInfo) -> bool
{
    ZoneScoped;

    m_layout.render(renderInfo.elementBox);

    // m_waveform has its own get_element_content_size() opinion (based on width). Placing it through a
    // layout helper (render_layout_element_pixels_vertical etc.) always takes max(requested, desired),
    // so it can never actually be constrained to less than its own desired height that way - it would
    // overflow into the control row whenever the desired height exceeds the space actually available
    // here. Render it directly with an exact box instead, and use a null-element spacer to keep
    // m_layout's own cursor in sync for the control row placed right after it.
    const ImRect layoutRect    = m_layout.get_element_rect();
    const float waveformHeight = std::max(0.0f, layoutRect.GetHeight() - s_controlHeight);

    m_waveform.render(ImRect(layoutRect.GetTL(), ImVec2(layoutRect.Max.x, layoutRect.Min.y + waveformHeight)));
    m_layout.render_spacer_pixels(0.0f, waveformHeight);

    m_layout.render_layout_element_pixels_vertical(&m_controlButtonsLayout, s_controlHeight);

    const bool createdComment = render_controls();

    if (renderInfo.isVisible)
    {
        handle_keyboard_controls(m_waveform.get_element_rect());
    }

    return createdComment;
}

auto audio_element::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
{
    return ImVec2(parentSize.x, m_waveform.get_element_content_size(parentSize).y + s_controlHeight);
}

auto audio_element::render_controls() -> bool
{
    ZoneScoped;

    m_filePositionText.set_text(fmt::format("{:02d}:{:02d}", static_cast<int>(m_filePosition) / 60, static_cast<int>(m_filePosition) % 60));
    m_fileDurationText.set_text(fmt::format("{:02d}:{:02d}", static_cast<int>(m_fileDuration) / 60, static_cast<int>(m_fileDuration) % 60));

    m_controlButtonsLayout.render_layout_element_pixels_horizontal(&m_filePositionText, s_buttonWidth);

    if (m_controlButtonsLayout.render_layout_element_pixels_horizontal(&m_previousFrameButton, s_buttonWidth))
    {
        prev_frame();
    }
    ImGui::SetItemTooltip("Previous Frame");

    if (m_controlButtonsLayout.render_layout_element_pixels_horizontal(&m_pauseButton, s_buttonWidth))
    {
        pause_file();
    }
    ImGui::SetItemTooltip("Pause");

    if (m_controlButtonsLayout.render_layout_element_pixels_horizontal(&m_playButton, s_buttonWidth))
    {
        play_file();
    }
    ImGui::SetItemTooltip("Play");

    const bool newComment = m_controlButtonsLayout.render_layout_element_pixels_horizontal(&m_addCommentButton, s_buttonWidth);
    ImGui::SetItemTooltip("Add Comment At Time");

    if (newComment)
    {
        pause_file();
    }

    if (m_controlButtonsLayout.render_layout_element_pixels_horizontal(&m_nextFrameButton, s_buttonWidth))
    {
        next_frame();
    }
    ImGui::SetItemTooltip("Next Frame");

    m_controlButtonsLayout.render_layout_element_pixels_horizontal(&m_fileDurationText, s_buttonWidth);

    return newComment;
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

auto audio_element::waveform_element::render_waveform_overlay(double plotTimeWidth) -> void
{
    if (std::shared_ptr<workspace_manager> workspaceManager = gluten::app::get()->get_manager_by_class<workspace_manager>())
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
                        const float plotWidthPercentageOfFileDuration = plotTimeWidth / m_fileDuration;
                        const bool showFullComment                    = plotWidthPercentageOfFileDuration <= 0.5f;
                        const auto userDisplayName                    = workspaceManager->get_user(comment.m_userId).m_displayName;

                        ImPlot::TagX(comment.m_timeStart, gluten::theme::supportInfo, showFullComment ? comment.m_comment.c_str() : userDisplayName.c_str());
                    }
                }
            }
        }
    }
}
