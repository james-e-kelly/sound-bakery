#pragma once

#include "gluten/elements/file_element.h"

namespace gluten
{
    class video_subsystem;
    struct video_info;

    enum class video_time_display
    {
        timecode,  //< HH:MM:SS:FF
        frames
    };

    /**
     * @brief Renders a video letterboxed in a viewport, a scrub timeline (ruler, marker lane, scrubber) and a
     * stats bar beneath it. Owns its own gluten::video_subsystem interaction, and all playback methods are
     * public so an app can drive it from its own controls (see video_transport).
     *
     * Deliberately draws no app-specific markers: render_timeline_markers() and render_frame_overlay() are
     * hooks for that.
     */
    class video_element : public file_element
    {
    public:
        video_element(const std::filesystem::path& videoFile);

        static auto can_handle_file(const std::filesystem::path& filePath) -> bool
        {
            const std::string extension = filePath.extension().string();

            static const std::unordered_set<std::string> videoExtensions = {
                ".yuv", ".y4m", ".m2ts", ".m2t", ".mts", ".mtv", ".ts", ".tsv",
                ".tsa", ".tts", ".trp", ".mpeg", ".mpg", ".mpe", ".mpeg2", ".m1v",
                ".m2v", ".mp2v", ".mpv", ".mpv2", ".mod", ".vob", ".vro", ".evob",
                ".evo", ".mpeg4", ".m4v", ".mp4", ".mp4v", ".mpg4", ".h264", ".avc",
                ".x264", ".264", ".hevc", ".h265", ".x265", ".265", ".ogv", ".ogm",
                ".ogx", ".mkv", ".mk3d", ".webm", ".avi", ".vfw", ".divx", ".3iv",
                ".xvid", ".nut", ".flic", ".fli", ".flc", ".nsv", ".gxf", ".mxf",
                ".wm", ".wmv", ".asf", ".dvr-ms", ".dvr", ".wtv", ".dv", ".hdv",
                ".flv", ".f4v", ".qt", ".mov", ".hdmov", ".rm", ".rmvb", ".3gpp",
                ".3gp", ".3gp2", ".3g2"};

            return videoExtensions.contains(extension);
        }

        auto get_file_play_position() const -> double override;
        auto get_file_duration() const -> double override;

        auto play_file() -> void override;
        auto pause_file() -> void override;
        auto stop_file() -> void;
        auto seek_to_position(double position) -> void override;
        auto prev_frame() -> void override;
        auto next_frame() -> void override;
        auto get_is_playing() -> bool override;

        auto get_file_path() const -> const std::filesystem::path& { return m_filePath; }
        auto get_info() const -> const video_info*;

        /**
         * @brief The container's frame rate, or 0 until mpv has probed it.
         */
        auto get_frame_rate() const -> double;

        /**
         * @brief A time in the current shared display mode: a frame timecode or a frame number.
         */
        auto format_time(double seconds) const -> std::string;

        auto get_element_content_size(const ImVec2& parentSize) -> ImVec2 const override;

        // View options are shared by every video, so a reviewer's setup carries from file to file.
        static auto get_time_display() -> video_time_display { return s_timeDisplay; }
        static auto set_time_display(video_time_display display) -> void { s_timeDisplay = display; }
        static auto get_show_safe_areas() -> bool { return s_showSafeAreas; }
        static auto set_show_safe_areas(bool show) -> void { s_showSafeAreas = show; }
        static auto get_show_timecode_burn_in() -> bool { return s_showTimecodeBurnIn; }
        static auto set_show_timecode_burn_in(bool show) -> void { s_showTimecodeBurnIn = show; }
        static auto get_show_frame_markers() -> bool { return s_showFrameMarkers; }
        static auto set_show_frame_markers(bool show) -> void { s_showFrameMarkers = show; }

        static constexpr float s_rulerHeight       = 20.0f;
        static constexpr float s_markerLaneHeight  = 22.0f;
        static constexpr float s_scrubHeight       = 18.0f;
        static constexpr float s_statusBarHeight   = 22.0f;
        static constexpr float s_timelineInset     = 12.0f;
        static constexpr float s_minViewportHeight = 160.0f;
        static constexpr float s_maxViewportHeight = 600.0f;

    protected:
        auto render_element(const element_render_info& renderInfo) -> bool override;

        /**
         * @brief Called after the marker lane background is drawn and before the playhead. timeToX maps a
         * file time to the shared timeline x.
         */
        virtual auto render_timeline_markers(const ImRect& markerLane, const std::function<float(double)>& timeToX) -> void {}

        /**
         * @brief Called over the picture when frame markers are shown. Draw whatever marks the playhead's moment.
         */
        virtual auto render_frame_overlay(const ImRect& frame) -> void {}

    private:
        auto get_aspect_ratio() const -> float;
        auto get_viewport_height(float width) const -> float;

        auto render_viewport(const ImRect& viewport) -> void;
        auto render_timeline(const ImRect& timeline) -> void;
        auto render_status_bar(const ImRect& statusBar) -> void;

        static inline std::weak_ptr<video_subsystem> m_videoSubsystem;

        static inline video_time_display s_timeDisplay = video_time_display::timecode;
        static inline bool s_showSafeAreas             = false;
        static inline bool s_showTimecodeBurnIn        = false;
        static inline bool s_showFrameMarkers          = true;

        uint32_t m_videoTexture = 0;
    };

    /**
     * @brief Formats seconds as HH:MM:SS:FF, counting frames at the nominal (rounded) rate.
     */
    auto format_frame_timecode(double seconds, double frameRate) -> std::string;
}  // namespace gluten
