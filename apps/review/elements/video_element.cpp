#include "video_element.h"

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
}

video_element::video_element(const std::filesystem::path& videoFile)
    : gluten::element(gluten::anchor_preset::stretch_full), m_videoFile(videoFile)
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
    m_playButton.set_element_hover_color(gluten::theme::carbon_g100::backgroundHover);
    m_pauseButton.set_element_hover_color(gluten::theme::carbon_g100::backgroundHover);
    m_playButton.set_element_active_color(gluten::theme::carbon_g100::backgroundActive);
    m_pauseButton.set_element_active_color(gluten::theme::carbon_g100::backgroundActive);

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

    const float remainingWidthMinusFinalText = elementRect.GetWidth() - (g_videoButtonsWidth * 4.0f) - g_gapBetweenButtonsAndText;

    const double videoPosition = 0.0;
    const double videoDuration = 16.0;
    const double videoPercent  = videoPosition / videoDuration;

    m_videoPositionText.set_text("00:00");
    m_videoDurationText.set_text("16:00");

    m_videoControlsLayout.render_layout_element_pixels_horizontal(nullptr, g_gapBetweenButtonsAndText);
    m_videoControlsLayout.render_layout_element_pixels_horizontal(&m_videoPositionText, g_videoButtonsWidth);
    m_videoControlsLayout.render_layout_element_pixels_horizontal(nullptr, remainingWidthMinusFinalText);
    m_videoControlsLayout.render_layout_element_pixels_horizontal(&m_videoDurationText, g_videoButtonsWidth);

    if (ImDrawList* const drawList = ImGui::GetWindowDrawList())
    {
        ImVec2 progressLineStart = videoControlsRect.Min;
        progressLineStart.x += (g_videoButtonsWidth * 3.0f) + g_gapBetweenButtonsAndText + g_progressLinePadding;
        progressLineStart.y += g_videoControlsHeight / 2.0f;

        ImVec2 progressLineEnd = progressLineStart;
        progressLineEnd.x += elementRect.GetWidth() - (g_videoButtonsWidth * 4.0f) - g_gapBetweenButtonsAndText - (g_progressLinePadding * 2.0f);

        drawList->AddLine(progressLineStart, progressLineEnd, ImGui::ColorConvertFloat4ToU32(gluten::theme::carbon_g100::textPrimary), g_progressLineThickness);

        const float progressX = progressLineStart.x + ((progressLineEnd.x - progressLineStart.x) * videoPercent);
        const float progressHandleStartX = progressX - (g_progressHandleWidth / 2.0f);
        const float progressHandleEndX   = progressHandleStartX + g_progressHandleWidth;
        const float progressHandleStartY = progressLineStart.y - (g_progressHandleHeight / 2.0f);
        const float progressHandleEndY   = progressHandleStartY + g_progressHandleHeight;


        drawList->AddRectFilled(ImVec2(progressHandleStartX, progressHandleStartY), ImVec2(progressHandleEndX, progressHandleEndY), ImGui::ColorConvertFloat4ToU32(gluten::theme::carbon_g100::interactive));
    }
}

auto video_element::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
{
    const ImVec2 videoSize = m_videoImage.get_element_content_size(parentSize);
    return ImVec2(videoSize.x, videoSize.y + g_videoControlsHeight);
}
