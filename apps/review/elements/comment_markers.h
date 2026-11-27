#pragma once

#include "data/comment_data.h"
#include "gluten/elements/audio_controls.h"

/**
 * @brief Timed review comments as the audio and video displays draw them: tags in a timeline marker lane and
 * previous / add / next buttons in the transport.
 */
namespace comment_markers
{
    auto color() -> ImVec4;
    auto initials_of(const std::string& displayName) -> std::string;

    /**
     * @brief This file's timed comments in the selected review, sorted by start time. Valid until the comment cache changes.
     */
    auto get_file_comments(int64_t fileId) -> std::vector<const comment_data*>;

    /**
     * @brief The comment whose span covers the given time, latest start first. Point comments cover a short hold after them.
     */
    auto find_comment_at(const std::vector<const comment_data*>& comments, double time) -> const comment_data*;

    /**
     * @brief Draws a tag per comment into the marker lane. Tags shrink to make room for the next one: full preview,
     * then initials, then a bare flag. Returns the time to seek to when a tag was clicked.
     *
     * @param compact Initials only, for when the view is zoomed out too far for previews to be readable.
     * @param formatTime How tooltips show a comment's time, matching the display the tags sit in.
     */
    auto render_tags(const ImRect& markerLane,
                     const std::vector<const comment_data*>& comments,
                     const std::function<float(double)>& timeToX,
                     bool compact,
                     const std::function<std::string(double)>& formatTime) -> std::optional<double>;

    /**
     * @brief Previous comment, "Comment at <positionLabel>" and next comment, for a transport's extras slot.
     * createdComment is set when the add button is pressed.
     */
    auto make_controls(int64_t fileId, const std::string& positionLabel, double position, std::function<void(double)> seek, bool& createdComment) -> gluten::transport_extras;
    auto controls_width(const std::string& positionLabel) -> float;
}  // namespace comment_markers
