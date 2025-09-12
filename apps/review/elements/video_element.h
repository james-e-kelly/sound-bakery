#pragma once

#include "elements/file_element.h"

class video_subsystem;

class video_element : public file_element
{
public:
    video_element(const std::filesystem::path& videoFile, int64_t fileId);

    auto get_video_position() const -> double
    {
        return m_filePosition;
    }

    static auto can_handle_file(const std::filesystem::path& filePath) -> bool
    {
        const std::string extension = filePath.extension().string();

        static const std::unordered_set<std::string> videoExtensions = {
            ".yuv",  ".y4m",   ".m2ts", ".m2t",    ".mts",   ".mtv",  ".ts",    ".tsv",
            ".tsa",  ".tts",   ".trp",  ".mpeg",   ".mpg",   ".mpe",  ".mpeg2", ".m1v",
            ".m2v",  ".mp2v",  ".mpv",  ".mpv2",   ".mod",   ".vob",  ".vro",   ".evob",
            ".evo",  ".mpeg4", ".m4v",  ".mp4",    ".mp4v",  ".mpg4", ".h264",  ".avc",
            ".x264", ".264",   ".hevc", ".h265",   ".x265",  ".265",  ".ogv",   ".ogm",
            ".ogx",  ".mkv",   ".mk3d", ".webm",   ".avi",   ".vfw",  ".divx",  ".3iv",
            ".xvid", ".nut",   ".flic", ".fli",    ".flc",   ".nsv",  ".gxf",   ".mxf",
            ".wm",   ".wmv",   ".asf",  ".dvr-ms", ".dvr",   ".wtv",  ".dv",    ".hdv",
            ".flv",  ".f4v",   ".qt",   ".mov",    ".hdmov", ".rm",   ".rmvb",  ".3gpp",
            ".3gp",  ".3gp2",  ".3g2"};

        return videoExtensions.contains(extension);
    }

    auto seek_to_position(double position) -> void override;

protected:
    auto render_element(const ImRect& elementRect) -> bool override;
    auto render_layouts(const ImRect& elementRect) -> void;
    auto render_timeline() -> void;
    auto render_comments() -> void;
    auto get_element_content_size(const ImVec2& parentSize) -> ImVec2 const override;

    auto get_file_play_position() const -> double override;
    auto get_file_duration() const -> double override;

    auto play_file() -> void override;
    auto pause_file() -> void override;
    auto prev_frame() -> void override;
    auto next_frame() -> void override;
    auto get_is_playing() -> bool override;

private:
    static inline std::weak_ptr<video_subsystem> m_videoSubsystem;
    uint32_t m_videoTexture = 0;

    gluten::image m_videoImage;

    gluten::layout m_videoControlsLayout = gluten::layout(gluten::layout_type::top_to_bottom, gluten::anchor_preset::stretch_full);
    gluten::layout m_videoTimelineLayout = gluten::layout(gluten::layout_type::left_to_right, gluten::anchor_preset::stretch_full);
    gluten::layout m_videoCommentsLayout = gluten::layout(gluten::layout_type::left_to_right, gluten::anchor_preset::stretch_full);
};
