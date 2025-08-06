#include "video_element.h"

#include "subsystems/video_subsystem.h"

auto video_element::render_element(const ImRect& elementRect) -> bool
{
	if (m_videoSubsystem.expired())
	{
        m_videoSubsystem = gluten::app::get()->get_subsystem_by_class<video_subsystem>();
		gluten::loading_spinner subsystemLoading;
        subsystemLoading.render(elementRect);
		return false;
	}

	std::shared_ptr<video_subsystem> videoSubsystem = m_videoSubsystem.lock();

	const uint32_t videoTexture = videoSubsystem->get_video_texture(m_videoFile.string());

	if (videoTexture == 0)
	{
        videoSubsystem->load_video(m_videoFile);
		gluten::loading_spinner videoLoading;
        videoLoading.render(elementRect);
		return false;
	}

	ImGui::Image((ImTextureID)videoTexture, ImVec2(1920 / 2, 1080 / 2));
}
