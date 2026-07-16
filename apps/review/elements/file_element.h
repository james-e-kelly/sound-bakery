#pragma once

#include "pch.h"

class file_element : public gluten::element
{
public:
    file_element(const gluten::anchor_preset& anchorPreset,
                 const std::filesystem::path& filePath,
                 const int64_t& fileId);

    auto pre_render_element() -> void override;
    auto render_element(const gluten::element_render_info& renderInfo) -> bool override;
    auto post_render_element() -> void override;

    auto get_file_position() const -> double
    {
        return m_filePosition;
    }

    static inline constexpr float s_buttonWidth             = 45.0f;
    static inline constexpr float s_controlHeight           = s_buttonWidth;
    static inline constexpr int s_controlButtonsCount       = 4;
    static inline constexpr float s_controlButtonsWidth     = s_buttonWidth * s_controlButtonsCount;

protected:
    auto render_controls() -> bool;

    auto handle_mouse_controls(const ImRect& contentArea) -> void;
    auto handle_keyboard_controls(const ImRect& contentArea) -> void;

    virtual auto get_file_play_position() const -> double = 0;
    virtual auto get_file_duration() const -> double      = 0;

    virtual auto play_file() -> void = 0;
    virtual auto pause_file() -> void = 0;
    virtual auto seek_to_position(double position) -> void = 0;
    virtual auto prev_frame() -> void                      = 0;
    virtual auto next_frame() -> void                      = 0;
    virtual auto get_is_playing() -> bool                  = 0;

    const std::filesystem::path m_filePath;
    int64_t m_fileId = -1;

    double m_filePosition = 0.0;
    double m_fileDuration = 1.0;
    double m_filePercent  = 0.0;

    gluten::icon_button m_playButton = gluten::icon_button("##Play", ICON_LC_PLAY, gluten::fonts::regular_lucide_icons);
    gluten::icon_button m_pauseButton = gluten::icon_button("##Pause", ICON_LC_PAUSE, gluten::fonts::regular_lucide_icons);
    gluten::icon_button m_previousFrameButton = gluten::icon_button("##PrevFrame", ICON_LC_CHEVRON_LEFT, gluten::fonts::regular_lucide_icons);
    gluten::icon_button m_nextFrameButton = gluten::icon_button("##NextFrame", ICON_LC_CHEVRON_RIGHT, gluten::fonts::regular_lucide_icons);
    gluten::icon_button m_addCommentButton = gluten::icon_button("##AddComment", ICON_LC_PLUS, gluten::fonts::regular_lucide_icons);
    gluten::text m_filePositionText;
    gluten::text m_fileDurationText;
    gluten::background m_fileBackground;
    gluten::layout m_controlButtonsLayout = gluten::layout(gluten::layout_type::left_to_right, gluten::anchor_preset::stretch_center);
};