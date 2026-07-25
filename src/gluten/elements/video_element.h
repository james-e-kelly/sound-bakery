#pragma once

#include "gluten/elements/file_element.h"
#include "gluten/elements/image.h"
#include "gluten/elements/layouts/layout.h"

namespace gluten
{
    class video_subsystem;

    /**
     * @brief Renders a video's current frame and a scrub timeline. Fully concrete - owns its own
     * gluten::video_subsystem interaction. All playback methods are public so an app can drive this
     * element directly (e.g. from its own control buttons) without subclassing it.
     *
     * Deliberately does not render any controls or any app-specific markers - those belong to whatever
     * app uses this element. render_video_overlay() is a hook for drawing markers on the timeline.
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
        auto seek_to_position(double position) -> void override;
        auto prev_frame() -> void override;
        auto next_frame() -> void override;
        auto get_is_playing() -> bool override;

        auto get_element_content_size(const ImVec2& parentSize) -> ImVec2 const override;

    protected:
        auto render_element(const element_render_info& renderInfo) -> bool override;
        auto render_layouts(const ImRect& elementRect) -> void;
        auto render_timeline() -> void;

        /**
         * @brief Called after the timeline row, with m_videoOverlayLayout laid out beneath it. Override
         * to draw markers on that row.
         */
        virtual auto render_video_overlay() -> void {}

        gluten::layout m_videoOverlayLayout = gluten::layout(gluten::layout_type::left_to_right, gluten::anchor_preset::stretch_full);

    private:
        static inline std::weak_ptr<video_subsystem> m_videoSubsystem;
        uint32_t m_videoTexture = 0;

        gluten::image m_videoImage;

        gluten::layout m_videoBottomLayout   = gluten::layout(gluten::layout_type::top_to_bottom, gluten::anchor_preset::stretch_full);
        gluten::layout m_videoTimelineLayout = gluten::layout(gluten::layout_type::left_to_right, gluten::anchor_preset::stretch_full);
    };
}  // namespace gluten
