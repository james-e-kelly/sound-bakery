#pragma once

#include "IconsLucide.h"
#include "gluten/elements/audio_element.h"
#include "gluten/elements/file_element.h"
#include "gluten/elements/icon_button.h"
#include "gluten/elements/text.h"

class audio_element : public gluten::file_element
{
public:
    audio_element(const std::filesystem::path& filePath, const int64_t fileId);

    static auto can_handle_file(const std::filesystem::path& filePath) -> bool
    {
        const std::string extension = filePath.extension().string();

        static const std::unordered_set<std::string> audioExtensions = {
            ".ac3", ".a52", ".eac3", ".mlp", ".dts", ".dts-hd", ".dtshd",
            ".true-hd", ".thd", ".truehd", ".thd+ac3", ".tta", ".pcm", ".wav",
            ".aiff", ".aif", ".aifc", ".amr", ".awb", ".au", ".snd",
            ".lpcm", ".ape", ".wv", ".shn", ".adts", ".adt", ".mpa",
            ".m1a", ".m2a", ".mp1", ".mp2", ".mp3", ".m4a", ".aac",
            ".flac", ".oga", ".ogg", ".opus", ".spx", ".mka", ".weba",
            ".wma", ".f4a", ".ra", ".ram", ".3ga", ".3ga2", ".ay",
            ".gbs", ".gym", ".hes", ".kss", ".nsf", ".nsfe", ".sap",
            ".spc", ".vgm", ".vgz", ".m3u", ".m3u8", ".pls", ".cue"};

        return audioExtensions.contains(extension);
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
     * @brief Extends Gluten's generic waveform display with review comment tags on the same plot.
     */
    class waveform_element : public gluten::audio_element
    {
    public:
        waveform_element(const std::filesystem::path& filePath, int64_t fileId);

    protected:
        auto render_waveform_overlay(double plotTimeWidth) -> void override;

    private:
        int64_t m_fileId = -1;
    };

    auto render_controls() -> bool;

    int64_t m_fileId = -1;

    waveform_element m_waveform;

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
