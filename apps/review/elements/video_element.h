#pragma once

#include "pch.h"

class video_subsystem;

class video_element : public gluten::element
{
public:
    video_element(const std::filesystem::path& videoFile)
        : m_videoFile(videoFile) {}

protected:
    auto render_element(const ImRect& elementRect) -> bool override;
    auto get_element_content_size() -> ImVec2 const override
    {
        return ImVec2(1920/2, 1080/2);
    }

private:
    const std::filesystem::path m_videoFile;
    static inline std::weak_ptr<video_subsystem> m_videoSubsystem;
};
