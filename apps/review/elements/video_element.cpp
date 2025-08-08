#include "video_element.h"

#include "managers/workspace_manager.h"
#include "subsystems/video_subsystem.h"

namespace
{
    constexpr float g_videoControlsHeight = 40.0f;
    constexpr float g_videoButtonsWidth   = g_videoControlsHeight;
    constexpr float g_gapBetweenButtonsAndText = 5.0f;
    constexpr float g_progressLineThickness    = 1.0f;
    constexpr float g_progressLinePadding      = 5.0f;
    constexpr float g_progressHandleWidth      = 10.0f;
    constexpr float g_progressHandleHeight     = g_videoControlsHeight / 2.0f;
    constexpr float g_minimumVideoPosition     = 0.0;
}

video_element::video_element(const std::filesystem::path& videoFile, int64_t fileId)
    : gluten::element(gluten::anchor_preset::stretch_full), m_videoFile(videoFile), m_fileId(fileId)
{
	if (m_videoSubsystem.expired())
    {
        m_videoSubsystem = gluten::app::get()->get_subsystem_by_class<video_subsystem>();
    }

	if (std::shared_ptr<video_subsystem> videoSubsystem = m_videoSubsystem.lock())
	{
        m_videoTexture = videoSubsystem->get_video_texture(m_videoFile.string());

		if (m_videoTexture == 0)
        {
            videoSubsystem->load_video(m_videoFile);
        }

		m_videoTexture = videoSubsystem->get_video_texture(m_videoFile.string());

        m_videoImage = gluten::image(m_videoTexture, 1920, 1080);
	}

    m_videoControlsBackground.set_element_background_color(gluten::theme::carbon_g100::background);
        m_addCommentButton.set_element_background_color(gluten::theme::carbon_g100::layer01);

    m_playButton.set_element_active_color(gluten::theme::carbon_g100::backgroundActive);
    m_pauseButton.set_element_active_color(gluten::theme::carbon_g100::backgroundActive);
    m_previousFrameButton.set_element_active_color(gluten::theme::carbon_g100::backgroundActive);
    m_nextFrameButton.set_element_active_color(gluten::theme::carbon_g100::backgroundActive);
    m_addCommentButton.set_element_active_color(gluten::theme::carbon_g100::layerActive01);

    m_playButton.set_element_hover_color(gluten::theme::carbon_g100::backgroundHover);
    m_pauseButton.set_element_hover_color(gluten::theme::carbon_g100::backgroundHover);
    m_previousFrameButton.set_element_hover_color(gluten::theme::carbon_g100::backgroundHover);
    m_nextFrameButton.set_element_hover_color(gluten::theme::carbon_g100::backgroundHover);
    m_addCommentButton.set_element_hover_color(gluten::theme::carbon_g100::layerHover01);

    m_videoPositionText.set_element_alignment(ImVec2(-0.f, -0.75f));
    m_videoDurationText.set_element_alignment(ImVec2(-0.f, -0.75f));
}

auto video_element::render_element(const ImRect& elementRect) -> bool
{
    gluten::imgui::scoped_id id(m_videoFile.string().c_str());
    //gluten::imgui::scoped_id randomId(std::rand());

    std::shared_ptr<video_subsystem> videoSubsystem = m_videoSubsystem.lock();
    if (!videoSubsystem)
    {
        return false;
    }

    m_videoImage.render(elementRect);

    ImRect videoControlsRect = elementRect;
    videoControlsRect.Min.y  = videoControlsRect.Max.y - g_videoControlsHeight;

    m_videoControlsBackground.render(videoControlsRect);

    //m_videoControlsLayout.set_element_padding(ImVec2(5.0f, 0.0f));
    m_videoControlsLayout.render(m_videoControlsBackground.get_element_rect());

    if (m_videoControlsLayout.render_layout_element_pixels_horizontal(&m_playButton, g_videoButtonsWidth))
    {
        videoSubsystem->play_video(m_videoFile);
    }

    if (m_videoControlsLayout.render_layout_element_pixels_horizontal(&m_pauseButton, g_videoButtonsWidth))
    {
        videoSubsystem->pause_video(m_videoFile);
    }

    if (m_videoControlsLayout.render_layout_element_pixels_horizontal(&m_previousFrameButton, g_videoButtonsWidth))
    {
        videoSubsystem->set_video_prev_frame(m_videoFile);
    }
    ImGui::SetItemTooltip("Previous Frame");

    if (m_videoControlsLayout.render_layout_element_pixels_horizontal(&m_nextFrameButton, g_videoButtonsWidth))
    {
        videoSubsystem->set_video_next_frame(m_videoFile);
    }
    ImGui::SetItemTooltip("Next Frame");

    if (m_videoControlsLayout.render_layout_element_pixels_horizontal(&m_addCommentButton, g_videoButtonsWidth))
    {
        if (std::shared_ptr<workspace_manager> workspaceManager = gluten::app::get()->get_manager_by_class<workspace_manager>())
        {
            gluten::data_source<user_settings_data> userSettingsData;

            new_comment_data newComment;
            newComment.m_reviewId = workspaceManager->get_selected_review().m_reviewId;
            newComment.m_fileId   = m_fileId;
            newComment.m_userId   = userSettingsData->m_loggedInUser.m_userId;
            newComment.m_comment  = "Comment for a video";

            workspaceManager->create_comment(newComment);
        }
    }
    ImGui::SetItemTooltip("Add Comment At Time");

    double videoPosition = videoSubsystem->get_video_play_position(m_videoFile);
    const double videoDuration = videoSubsystem->get_video_duration(m_videoFile);
    const double videoPercent  = videoPosition / videoDuration;

    m_videoPositionText.set_text(fmt::format("{:02d}:{:02d}", static_cast<int>(videoPosition) / 60, static_cast<int>(videoPosition) % 60));
    m_videoDurationText.set_text(fmt::format("{:02d}:{:02d}", static_cast<int>(videoDuration) / 60, static_cast<int>(videoDuration) % 60));

    m_videoControlsLayout.render_layout_element_pixels_horizontal(nullptr, g_gapBetweenButtonsAndText);
    m_videoControlsLayout.render_layout_element_pixels_horizontal(&m_videoPositionText, g_videoButtonsWidth);

    const float timelineWidth = m_videoControlsLayout.get_remaining_layout_size().x - g_videoButtonsWidth - g_gapBetweenButtonsAndText;

    ImVec2 progressLineStart = m_videoControlsLayout.get_current_layout_pos();
    progressLineStart.y += g_videoControlsHeight / 2.0f;

    m_videoControlsLayout.render_layout_element_pixels_horizontal(nullptr, timelineWidth);

    ImVec2 progressLineEnd = m_videoControlsLayout.get_current_layout_pos();
    progressLineEnd.y += g_videoControlsHeight / 2.0f;

    if (ImDrawList* const drawList = ImGui::GetWindowDrawList())
    {
        drawList->AddLine(
            ImVec2(progressLineStart.x + (ImGui::GetStyle().GrabMinSize / 2.0f), progressLineStart.y), 
            ImVec2(progressLineEnd.x - (ImGui::GetStyle().GrabMinSize / 2.0f), progressLineEnd.y),
            ImGui::ColorConvertFloat4ToU32(gluten::theme::carbon_g100::textPrimary),
            g_progressLineThickness);

        const ImGuiID videoGrabHandleId = ImGui::GetID("##VideoDragHandle");
        ImGui::KeepAliveID(videoGrabHandleId);

        ImRect grabRect(progressLineStart.x, progressLineStart.y - (g_videoControlsHeight / 3.0f), progressLineEnd.x,
               progressLineEnd.y + (g_videoControlsHeight / 3.0f));

        const bool hovered = ImGui::ItemHoverable(grabRect, videoGrabHandleId, ImGuiSliderFlags_NoInput);

        const bool clicked     = hovered && ImGui::IsMouseClicked(0, ImGuiInputFlags_None, videoGrabHandleId);
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
        if (ImGui::SliderBehavior(grabRect, videoGrabHandleId, ImGuiDataType_Double, &videoPosition, &g_minimumVideoPosition, &videoDuration, "%f", ImGuiSliderFlags_None, &outDrag))
        {
            ImGui::MarkItemEdited(videoGrabHandleId);
            videoSubsystem->set_video_play_position(m_videoFile, videoPosition);
        }

        if (outDrag.Max.x > outDrag.Min.x)
        {
            drawList->AddRectFilled(outDrag.Min, outDrag.Max,ImGui::ColorConvertFloat4ToU32(gluten::theme::carbon_g100::interactive), 0.0f);
        }
    }

    m_videoControlsLayout.render_layout_element_pixels_horizontal(&m_videoDurationText, g_videoButtonsWidth);
}

auto video_element::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
{
    const ImVec2 videoSize = m_videoImage.get_element_content_size(parentSize);
    return ImVec2(videoSize.x, videoSize.y + g_videoControlsHeight);
}
