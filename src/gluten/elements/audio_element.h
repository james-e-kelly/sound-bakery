#pragma once

#include "gluten/elements/element.h"
#include "gluten/elements/layouts/layout.h"

namespace gluten
{
    enum class waveform_overlay
    {
        none,
        loudness,  //< Short-term LUFS line plus an integrated LUFS reference line
        bands      //< Low/mid/high band levels in dB
    };

    struct waveform_loudness_stats
    {
        std::optional<double> integrated;    //< LUFS
        std::optional<double> range;         //< LU
        std::optional<double> truePeak;      //< dBTP
        std::optional<double> samplePeak;    //< dBFS
        std::optional<double> shorttermMax;  //< LUFS
    };

    /**
     * @brief Renders an audio file as a timeline: a time ruler, a marker lane, one lane per channel
     * (peak and RMS), a loudness overlay, the playhead, and a status bar of loudness stats.
     *
     * Handles basic playback interaction (click or scrub the ruler/waveform to seek, right click to play/pause).
     * Deliberately does not render any transport controls or app-specific markers - those belong to whatever
     * app uses this element. Apps draw markers into the marker lane from render_waveform_overlay().
     */
    class audio_element : public element
    {
    public:
        audio_element(const std::filesystem::path& filePath);

        auto get_file_path() const -> const std::filesystem::path& { return m_filePath; }
        auto get_position() const -> double;
        auto get_duration() const -> double;
        auto is_playing() const -> bool;

        auto play() -> void;
        auto pause() -> void;
        auto seek(double position) -> void;

        /**
         * @brief View settings are remembered per file, since elements are usually rebuilt every frame.
         */
        auto set_overlay(waveform_overlay overlay) -> void;
        auto get_overlay() const -> waveform_overlay;
        auto set_render_mid_side(bool renderMidSide) -> void;
        auto get_render_mid_side() const -> bool;

        auto get_channel_count() const -> std::size_t { return m_channelCount; }
        auto get_sample_rate() const -> std::size_t { return m_sampleRate; }
        auto get_loudness_stats() const -> const waveform_loudness_stats& { return m_loudnessStats; }

        /**
         * @brief Peak level of each channel around a time, read from the cached waveform. Cheap enough for meters.
         */
        auto get_channel_peaks_at(double position) const -> std::vector<float>;

        auto get_element_content_size(const ImVec2& parentSize) -> ImVec2 const override;

        static constexpr float s_rulerHeight      = 20.0f;
        static constexpr float s_markerLaneHeight = 22.0f;
        static constexpr float s_trackHeaderWidth = 28.0f;
        static constexpr float s_statusBarHeight  = 22.0f;

    protected:
        auto render_element(const element_render_info& renderInfo) -> bool override;

        /**
         * @brief Called inside the waveform's ImPlot context (axes set to time/linear volume), after the
         * waveform and before the playhead. Override to draw markers into the marker lane.
         */
        virtual auto render_waveform_overlay(const ImRect& markerLane, double plotTimeWidth) -> void {}

        const std::filesystem::path m_filePath;

        double m_filePosition = 0.0;
        double m_fileDuration = 1.0;

        gluten::background m_audioBackground;
        gluten::layout m_waveformAndLoudnessLayout = gluten::layout(gluten::layout_type::left_to_right, gluten::anchor_preset::stretch_full);

    private:
        auto render_waveform() -> void;
        auto render_status_bar(const ImRect& statusBar) -> void;
        auto get_audio_height(float width) -> float;

        std::size_t m_channelCount = 0;
        std::size_t m_sampleRate = 0;
        waveform_loudness_stats m_loudnessStats;
    };

    /**
     * @brief Draws text with every digit given the same advance, so changing numbers (timecodes, meters) don't jitter.
     *
     * @return The width of the drawn text.
     */
    auto draw_tabular_text(ImDrawList* drawList, ImFont* font, float fontSize, ImVec2 position, ImU32 color, std::string_view text) -> float;
    auto calc_tabular_text_width(ImFont* font, float fontSize, std::string_view text) -> float;

    /**
     * @brief Ruler tick spacing: a labelled major tick every `major` seconds, split into `minorDivisions`.
     */
    struct ruler_step
    {
        double major;
        int minorDivisions;
    };

    /**
     * @brief The finest ruler step whose major ticks are at least labelWidth pixels apart.
     */
    auto pick_ruler_step(double pixelsPerSecond, float labelWidth) -> ruler_step;

    /**
     * @brief Formats seconds as mm:ss.mmm (or mm:ss when showMilliseconds is false).
     */
    auto format_timecode(double seconds, bool showMilliseconds = true) -> std::string;
}  // namespace gluten
