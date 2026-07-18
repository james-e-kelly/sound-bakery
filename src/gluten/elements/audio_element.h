#pragma once

#include "gluten/elements/element.h"
#include "gluten/elements/layouts/layout.h"

namespace gluten
{
    /**
     * @brief Renders the waveform, LUFS/dB overlays and playhead for an audio file, and
     * handles basic playback interaction (click to seek, right click to play/pause).
     *
     * Deliberately does not render any playback controls (play/pause buttons, timecodes, etc.)
     * or any app-specific markers on the waveform - those belong to whatever app uses this element.
     */
    class audio_element : public element
    {
    public:
        audio_element(const std::filesystem::path& filePath);

        auto get_position() const -> double;
        auto get_duration() const -> double;
        auto is_playing() const -> bool;

        auto play() -> void;
        auto pause() -> void;
        auto seek(double position) -> void;

        auto get_element_content_size(const ImVec2& parentSize) -> ImVec2 const override;

    protected:
        auto render_element(const element_render_info& renderInfo) -> bool override;

        /**
         * @brief Called inside the waveform's ImPlot context (axes set to time/linear volume), after the
         * playhead has been drawn. Override to draw extra markers on the same plot.
         */
        virtual auto render_waveform_overlay(double plotTimeWidth) -> void {}

        const std::filesystem::path m_filePath;

        double m_filePosition = 0.0;
        double m_fileDuration = 1.0;

        gluten::background m_audioBackground;
        gluten::layout m_waveformAndLoudnessLayout = gluten::layout(gluten::layout_type::left_to_right, gluten::anchor_preset::stretch_full);

    private:
        auto render_waveform() -> void;
        auto get_audio_height(float width) -> float;
    };
}  // namespace gluten
