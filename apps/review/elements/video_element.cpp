#include "video_element.h"

#include "managers/workspace_manager.h"
#include "subsystems/video_subsystem.h"

namespace
{
    constexpr float g_videoControlRowHeight = 30.0f;
    constexpr float g_videoControlRowHalfHeight = g_videoControlRowHeight / 2.0f;
    constexpr int g_videoControlRows           = 3;
    constexpr float g_totalVideoControlsHeight = g_videoControlRowHeight * g_videoControlRows;


    constexpr float g_videoButtonsWidth   = g_videoControlRowHeight;
    constexpr int g_videoButtonsCount          = 5;
    constexpr float g_totalVideoButtonsWidth   = g_videoButtonsWidth * g_videoButtonsCount;
    constexpr float g_progressLineThickness    = 1.0f;
    constexpr float g_progressLinePadding      = 5.0f;
    constexpr float g_progressHandleWidth      = 10.0f;
    constexpr float g_progressHandleHeight     = g_videoControlRowHeight / 2.0f;
    constexpr float g_minimumVideoPosition     = 0.0;
    constexpr float g_commentBubbleRadius      = 10.0f;
    constexpr float g_commentBubbleDiamter     = g_commentBubbleRadius * 2.0f;
}

video_element::video_element(const std::filesystem::path& videoFile, int64_t fileId)
    : file_element(gluten::anchor_preset::stretch_full, videoFile, fileId)
{
	if (m_videoSubsystem.expired())
    {
        m_videoSubsystem = gluten::app::get()->get_subsystem_by_class<video_subsystem>();
    }

	if (std::shared_ptr<video_subsystem> videoSubsystem = m_videoSubsystem.lock())
	{
        m_videoTexture = videoSubsystem->get_video_texture(m_filePath.string());

		if (m_videoTexture == 0)
        {
            videoSubsystem->load_video(m_filePath);
        }

		m_videoTexture = videoSubsystem->get_video_texture(m_filePath.string());

        m_videoImage = gluten::image(m_videoTexture, 1920, 1080);
	}

    m_videoControlsLayout.set_element_background_color(gluten::theme::carbon_g100::background);

    m_videoCommentsLayout.set_element_padding(ImVec2(g_commentBubbleDiamter, 0.0f));
    m_videoTimelineLayout.set_element_padding(ImVec2(g_commentBubbleDiamter, 0.0f));
}

auto video_element::render_element(const ImRect& elementRect) -> bool
{
    file_element::render_element(elementRect);

    std::shared_ptr<video_subsystem> videoSubsystem = m_videoSubsystem.lock();
    if (!videoSubsystem)
    {
        return false;
    }

    m_videoImage.render(elementRect);

    render_layouts(elementRect);
    const bool newComment = render_controls();
    render_timeline();
    render_comments();
    return newComment;
}

auto video_element::render_timeline() -> void
{
    const float timelineWidth = m_videoTimelineLayout.get_element_rect().GetWidth();

    ImVec2 progressLineStart = m_videoTimelineLayout.get_element_rect().GetTL();
    progressLineStart.y += g_videoControlRowHalfHeight;

    ImVec2 progressLineEnd = progressLineStart;
    progressLineEnd.x += timelineWidth;

    if (ImDrawList* const drawList = ImGui::GetWindowDrawList())
    {
        drawList->AddLine(ImVec2(progressLineStart.x, progressLineStart.y),
                          ImVec2(progressLineEnd.x, progressLineEnd.y),
                          ImGui::ColorConvertFloat4ToU32(gluten::theme::carbon_g100::textPrimary),
                          g_progressLineThickness);

        const ImGuiID videoGrabHandleId = ImGui::GetID("##VideoDragHandle");
        ImGui::KeepAliveID(videoGrabHandleId);

        ImRect grabRect(progressLineStart.x - (ImGui::GetStyle().GrabMinSize / 2.0f),
                        progressLineStart.y - (g_videoControlRowHeight / 3.0f), 
                        progressLineEnd.x + (ImGui::GetStyle().GrabMinSize / 2.0f),
                        progressLineEnd.y + (g_videoControlRowHeight / 3.0f));

        const bool hovered = ImGui::ItemHoverable(grabRect, videoGrabHandleId, ImGuiSliderFlags_NoInput);

        const bool clicked    = hovered && ImGui::IsMouseClicked(0, ImGuiInputFlags_None, videoGrabHandleId);
        const bool makeActive = (clicked || ImGui::GetCurrentContext()->NavActivateId == videoGrabHandleId);

        if (makeActive && clicked)
        {
            ImGui::SetKeyOwner(ImGuiKey_MouseLeft, videoGrabHandleId);
        }

        if (makeActive)
        {
            ImGui::SetActiveID(videoGrabHandleId, ImGui::GetCurrentWindow());
            ImGui::SetFocusID(videoGrabHandleId, ImGui::GetCurrentWindow());
            ImGui::FocusWindow(ImGui::GetCurrentWindow());
            ImGui::GetCurrentContext()->ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
        }

        ImRect outDrag;
        if (ImGui::SliderBehavior(grabRect, videoGrabHandleId, ImGuiDataType_Double, &m_filePosition,
                                  &g_minimumVideoPosition, &m_fileDuration, "%f", ImGuiSliderFlags_None, &outDrag))
        {
            ImGui::MarkItemEdited(videoGrabHandleId);
            m_videoSubsystem.lock()->set_video_play_position(m_filePath, m_filePosition);
        }

        if (outDrag.Max.x > outDrag.Min.x)
        {
            drawList->AddRectFilled(outDrag.Min, outDrag.Max,
                                    ImGui::ColorConvertFloat4ToU32(gluten::theme::carbon_g100::interactive), 0.0f);
        }        
    }
}

auto video_element::render_comments() -> void
{
    if (std::shared_ptr<workspace_manager> workspaceManager =
            gluten::app::get()->get_manager_by_class<workspace_manager>())
    {
        if (ImDrawList* const drawList = ImGui::GetWindowDrawList())
        {
            ImVec2 leftMiddle = m_videoCommentsLayout.get_element_rect().GetTL();
            leftMiddle.y += g_videoControlRowHeight / 2.0f;

            ImVec2 rightMiddle = m_videoCommentsLayout.get_element_rect().GetTR();
            rightMiddle.y += g_videoControlRowHeight / 2.0f;

            drawList->AddLine(leftMiddle, rightMiddle,
                              ImGui::ColorConvertFloat4ToU32(gluten::theme::carbon_g100::textPrimary), 1.0f);

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
                            const float commentPosition = leftMiddle.x + (commentTimelineWidth * (comment.m_timeStart / m_fileDuration));

                            if (std::abs(commentPosition) < 10000.0f)
                            {
                                ImRect circleRect(
                                    commentPosition - g_commentBubbleRadius, leftMiddle.y - g_commentBubbleRadius,
                                    commentPosition + g_commentBubbleRadius, leftMiddle.y + g_commentBubbleRadius);

                                drawList->AddCircleFilled(ImVec2(commentPosition, leftMiddle.y), g_commentBubbleRadius,
                                                          ImGui::ColorConvertFloat4ToU32(gluten::theme::purple50));
                                ImGui::ItemAdd(circleRect, ImGui::GetID(comment.m_commentId));
                                ImGui::SetItemTooltip(comment.m_comment.c_str());

                                if (ImGui::IsItemClicked())
                                {
                                    m_videoSubsystem.lock()->set_video_play_position(m_filePath, comment.m_timeStart);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

auto video_element::render_layouts(const ImRect& elementRect) -> void
{
    ImRect videoControlsRect = elementRect;
    videoControlsRect.Min.y  = videoControlsRect.Max.y - g_totalVideoControlsHeight;
    m_videoControlsLayout.render(videoControlsRect);

    m_videoControlsLayout.render_layout_element_pixels_vertical(&m_controlButtonsLayout, g_videoControlRowHeight);
    m_videoControlsLayout.render_layout_element_pixels_vertical(&m_videoTimelineLayout, g_videoControlRowHeight);
    m_videoControlsLayout.render_layout_element_pixels_vertical(&m_videoCommentsLayout, g_videoControlRowHeight);
}

auto video_element::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
{
    const ImVec2 videoSize = m_videoImage.get_element_content_size(parentSize);
    return ImVec2(videoSize.x, videoSize.y + g_totalVideoControlsHeight);
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