#pragma once

#include "gluten/elements/element.h"

namespace gluten
{
    /**
     * @brief Base for a playable media element (audio, video, ...). Tracks playback position/duration
     * and handles the most basic playback interaction (space to play/pause, double-click to play,
     * right-click to toggle play/pause on the content area).
     *
     * Deliberately renders no UI of its own (no buttons, no timecodes) and has no notion of an identifier
     * or metadata tying it into a particular app's data model - that belongs to whatever app extends this.
     */
    class file_element : public element
    {
    public:
        file_element(const anchor_preset& anchorPreset, const std::filesystem::path& filePath);

        auto pre_render_element() -> void override;
        auto post_render_element() -> void override;

        auto get_file_position() const -> double
        {
            return m_filePosition;
        }

    protected:
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

        double m_filePosition = 0.0;
        double m_fileDuration = 1.0;
        double m_filePercent  = 0.0;
    };
}  // namespace gluten
