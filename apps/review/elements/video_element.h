#pragma once

#include "pch.h"

class video_subsystem;

class video_element : public gluten::element
{
public:
    video_element(const std::filesystem::path& videoFile);

protected:
    auto render_element(const ImRect& elementRect) -> bool override;

private:
    const std::filesystem::path m_videoFile;
    static inline std::weak_ptr<video_subsystem> m_videoSubsystem;
    uint32_t m_videoTexture = 0;
};
