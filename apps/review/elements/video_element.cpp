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
	}
}

auto video_element::render_element(const ImRect& elementRect) -> bool
{
    if (m_videoTexture > 0)
    {
	    gluten::image videoImage(m_videoTexture, 1920, 1080);
        //videoImage.set_element_anchor_preset(gluten::anchor_preset::stretch_top);
        return videoImage.render(elementRect);
    }
    return false;
}
