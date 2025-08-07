#pragma once

#include "pch.h"

class video_subsystem;

class video_element : public gluten::element
{
public:
    video_element(const std::filesystem::path& videoFile);

protected:
    auto render_element(const ImRect& elementRect) -> bool override;
    auto get_element_content_size(const ImVec2& parentSize) -> ImVec2 const override;

private:
    const std::filesystem::path m_videoFile;
    static inline std::weak_ptr<video_subsystem> m_videoSubsystem;
    uint32_t m_videoTexture = 0;

    gluten::image m_videoImage;
    gluten::background m_videoControlsBackground;
    gluten::layout m_videoControlsLayout = gluten::layout(gluten::layout_type::left_to_right, gluten::anchor_preset::stretch_full);
    gluten::icon_button m_playButton = gluten::icon_button("##Play", ICON_LC_PLAY, gluten::fonts::regular_lucide_icons);
    gluten::icon_button m_pauseButton = gluten::icon_button("##Pause", ICON_LC_PAUSE, gluten::fonts::regular_lucide_icons);
    gluten::text m_videoPositionText;
    gluten::text m_videoDurationText;
};
