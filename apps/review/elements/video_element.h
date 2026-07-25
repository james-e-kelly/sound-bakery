#pragma once

#include "IconsLucide.h"
#include "gluten/elements/file_element.h"
#include "gluten/elements/icon_button.h"
#include "gluten/elements/text.h"
#include "gluten/elements/video_element.h"

class video_element : public gluten::file_element
{
public:
    video_element(const std::filesystem::path& videoFile, int64_t fileId);

    static auto can_handle_file(const std::filesystem::path& filePath) -> bool
    {
        return gluten::video_element::can_handle_file(filePath);
    }

    auto seek_to_position(double position) -> void override;

protected:
    auto render_element(const gluten::element_render_info& renderInfo) -> bool override;
    auto get_element_content_size(const ImVec2& parentSize) -> ImVec2 const override;

    auto get_file_play_position() const -> double override;
    auto get_file_duration() const -> double override;

    auto play_file() -> void override;
    auto pause_file() -> void override;
    auto prev_frame() -> void override;
    auto next_frame() -> void override;
    auto get_is_playing() -> bool override;

private:
    static inline constexpr float s_buttonWidth         = 45.0f;
    static inline constexpr float s_controlHeight       = s_buttonWidth;
    static inline constexpr int s_controlButtonsCount   = 4;  // prev, pause, play, next
    static inline constexpr float s_controlButtonsWidth = s_buttonWidth * s_controlButtonsCount;

    /**
     * @brief Extends Gluten's generic video display with review comment bubbles on the timeline.
     */
    class inner_video_element : public gluten::video_element
    {
    public:
        inner_video_element(const std::filesystem::path& filePath, int64_t fileId);

    protected:
        auto render_video_overlay() -> void override;

    private:
        int64_t m_fileId = -1;
    };

    auto render_controls() -> bool;

    int64_t m_fileId = -1;

    inner_video_element m_video;

    gluten::icon_button m_playButton          = gluten::icon_button("##Play", ICON_LC_PLAY, gluten::fonts::regular_lucide_icons);
    gluten::icon_button m_pauseButton         = gluten::icon_button("##Pause", ICON_LC_PAUSE, gluten::fonts::regular_lucide_icons);
    gluten::icon_button m_previousFrameButton = gluten::icon_button("##PrevFrame", ICON_LC_CHEVRON_LEFT, gluten::fonts::regular_lucide_icons);
    gluten::icon_button m_nextFrameButton     = gluten::icon_button("##NextFrame", ICON_LC_CHEVRON_RIGHT, gluten::fonts::regular_lucide_icons);
    gluten::icon_button m_addCommentButton    = gluten::icon_button("##AddComment", ICON_LC_PLUS, gluten::fonts::regular_lucide_icons);

    gluten::text m_filePositionText;
    gluten::text m_fileDurationText;

    gluten::layout m_layout               = gluten::layout(gluten::layout_type::top_to_bottom, gluten::anchor_preset::stretch_full);
    gluten::layout m_controlButtonsLayout = gluten::layout(gluten::layout_type::left_to_right, gluten::anchor_preset::stretch_center);
};
