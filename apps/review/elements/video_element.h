#pragma once

#include "gluten/elements/audio_controls.h"
#include "gluten/elements/file_element.h"
#include "gluten/elements/layouts/layout.h"
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
    /**
     * @brief Extends Gluten's video display with review comment tags on the timeline and the comment at the
     * playhead over the frame.
     */
    class inner_video_element : public gluten::video_element
    {
    public:
        inner_video_element(const std::filesystem::path& filePath, int64_t fileId);

    protected:
        auto render_timeline_markers(const ImRect& markerLane, const std::function<float(double)>& timeToX) -> void override;
        auto render_frame_overlay(const ImRect& frame) -> void override;

    private:
        int64_t m_fileId = -1;
    };

    auto make_comment_controls(bool& createdComment) -> gluten::transport_extras;

    int64_t m_fileId = -1;

    inner_video_element m_video;

    gluten::layout m_layout = gluten::layout(gluten::layout_type::top_to_bottom, gluten::anchor_preset::stretch_full);
};
