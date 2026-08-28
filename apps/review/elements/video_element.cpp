#include "video_element.h"

#include "managers/workspace_manager.h"

namespace
{
    constexpr float g_commentBubbleRadius = 10.0f;
}

video_element::video_element(const std::filesystem::path& videoFile, int64_t fileId)
    : gluten::file_element(gluten::anchor_preset::stretch_full, videoFile), m_fileId(fileId), m_video(videoFile, fileId)
{
    m_layout.set_layout_spacing(gluten::theme::space04);
    m_layout.set_element_padding(gluten::theme::insetFrame);

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
    m_controlButtonsLayout.set_element_rounding(gluten::theme::radiusMd);
    m_controlButtonsLayout.set_element_background_color(gluten::theme::layer02);
    m_controlButtonsLayout.set_element_max_size(ImVec2(0.0f, s_controlButtonsWidth));
}

auto video_element::render_element(const gluten::element_render_info& renderInfo) -> bool
{
    ZoneScoped;

    m_layout.render(renderInfo.elementBox);

    // m_video has its own get_element_content_size() opinion (video image aspect ratio + its internal
    // timeline/overlay rows). Placing it through a layout helper (render_layout_element_pixels_vertical
    // etc.) always takes max(requested, desired), so it can never actually be constrained to less than
    // its own desired height that way - it would overflow into the control row whenever the desired
    // height exceeds the space actually available here. Render it directly with an exact box instead,
    // and use a null-element spacer to keep m_layout's own cursor in sync for the control row placed
    // right after it.
    const ImRect layoutRect = m_layout.get_element_rect();
    const float videoHeight = std::max(0.0f, layoutRect.GetHeight() - s_controlHeight);

    m_video.render(ImRect(layoutRect.GetTL(), ImVec2(layoutRect.Max.x, layoutRect.Min.y + videoHeight)));
    m_layout.render_spacer_pixels(0.0f, videoHeight);

    m_layout.render_layout_element_pixels_vertical(&m_controlButtonsLayout, s_controlHeight);

    const bool createdComment = render_controls();

    if (renderInfo.isVisible)
    {
        handle_keyboard_controls(m_video.get_element_rect());
    }

    return createdComment;
}

auto video_element::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
{
    return ImVec2(parentSize.x, m_video.get_element_content_size(parentSize).y + s_controlHeight);
}

auto video_element::render_controls() -> bool
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

auto video_element::get_file_play_position() const -> double
{
    return m_video.get_file_play_position();
}

auto video_element::get_file_duration() const -> double
{
    return m_video.get_file_duration();
}

auto video_element::play_file() -> void
{
    m_video.play_file();
}

auto video_element::pause_file() -> void
{
    m_video.pause_file();
}

auto video_element::prev_frame() -> void
{
    m_video.prev_frame();
}

auto video_element::next_frame() -> void
{
    m_video.next_frame();
}

auto video_element::get_is_playing() -> bool
{
    return m_video.get_is_playing();
}

auto video_element::seek_to_position(double position) -> void
{
    m_video.seek_to_position(position);
}

video_element::inner_video_element::inner_video_element(const std::filesystem::path& filePath, int64_t fileId)
    : gluten::video_element(filePath), m_fileId(fileId)
{
}

auto video_element::inner_video_element::render_video_overlay() -> void
{
    if (std::shared_ptr<workspace_manager> workspaceManager = gluten::app::get()->get_manager_by_class<workspace_manager>())
    {
        if (ImDrawList* const drawList = ImGui::GetWindowDrawList())
        {
            const ImRect overlayRect = m_videoOverlayLayout.get_element_rect();
            const float rowMiddleY   = overlayRect.Min.y + (overlayRect.GetHeight() / 2.0f);

            const ImVec2 leftMiddle(overlayRect.Min.x, rowMiddleY);
            const ImVec2 rightMiddle(overlayRect.Max.x, rowMiddleY);

            drawList->AddLine(leftMiddle, rightMiddle, ImGui::ColorConvertFloat4ToU32(gluten::theme::textPrimary), 1.0f);

            const auto& comments = workspaceManager->get_all_comments_for_review(workspaceManager->get_selected_review().m_reviewId);

            if (comments.has_data())
            {
                for (const auto& comment : comments.m_cache)
                {
                    if (comment.m_fileId == m_fileId)
                    {
                        if (comment.m_timeStart >= 0.0)
                        {
                            const float commentTimelineWidth = rightMiddle.x - leftMiddle.x;
                            const float commentPosition      = leftMiddle.x + (commentTimelineWidth * (comment.m_timeStart / m_fileDuration));

                            if (std::abs(commentPosition) < 10000.0f)
                            {
                                ImRect circleRect(
                                    commentPosition - g_commentBubbleRadius, leftMiddle.y - g_commentBubbleRadius,
                                    commentPosition + g_commentBubbleRadius, leftMiddle.y + g_commentBubbleRadius);

                                drawList->AddCircleFilled(ImVec2(commentPosition, leftMiddle.y), g_commentBubbleRadius,
                                                          ImGui::ColorConvertFloat4ToU32(gluten::theme::supportWarning));
                                ImGui::ItemAdd(circleRect, ImGui::GetID(comment.m_commentId));
                                ImGui::SetItemTooltip(comment.m_comment.c_str());

                                if (ImGui::IsItemClicked())
                                {
                                    seek_to_position(comment.m_timeStart);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
