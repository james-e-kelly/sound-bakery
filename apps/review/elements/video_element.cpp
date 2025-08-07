#include "video_element.h"

#include "subsystems/video_subsystem.h"

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
}

auto video_element::render_element(const ImRect& elementRect) -> bool
{
    return m_videoImage.render(elementRect);
}

auto video_element::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
{
    const ImVec2 videoSize = m_videoImage.get_element_content_size(parentSize);
    return ImVec2(videoSize.x, videoSize.y + 40.0f);
}
