#include "video_element.h"

#include "elements/comment_markers.h"
#include "gluten/elements/audio_element.h"
#include "gluten/elements/video_controls.h"
#include "managers/workspace_manager.h"

video_element::video_element(const std::filesystem::path& videoFile, int64_t fileId)
    : gluten::file_element(gluten::anchor_preset::stretch_full, videoFile), m_fileId(fileId), m_video(videoFile, fileId)
{
    m_layout.set_layout_spacing(gluten::theme::space04);
    m_layout.set_element_padding(ImVec2(gluten::theme::space08, gluten::theme::space08));
}

auto video_element::render_element(const gluten::element_render_info& renderInfo) -> bool
{
    ZoneScoped;

    m_layout.render(renderInfo.elementBox);

    // m_video has its own get_element_content_size() opinion (based on width), and layout helpers always take
    // max(requested, desired), so it is placed with an exact box instead and a spacer keeps m_layout's cursor in sync.
    const ImRect layoutRect = m_layout.get_element_rect();

    bool createdComment                   = false;
    const gluten::transport_extras extras = make_comment_controls(createdComment);

    const float transportHeight = gluten::video_transport::get_height(layoutRect.GetWidth(), extras.width);
    const ImRect transportRect(layoutRect.Min, ImVec2(layoutRect.Max.x, layoutRect.Min.y + transportHeight));
    const float videoTop = transportRect.Max.y + gluten::theme::space04;
    const ImRect videoRect(ImVec2(layoutRect.Min.x, videoTop), ImVec2(layoutRect.Max.x, std::max(videoTop, layoutRect.Max.y)));

    m_layout.render_spacer_pixels(0.0f, videoTop - layoutRect.Min.y - gluten::theme::space04);

    m_video.render(videoRect);
    m_layout.render_spacer_pixels(0.0f, videoRect.GetHeight());

    gluten::video_transport::render(transportRect, m_video, extras);

    if (createdComment)
    {
        pause_file();
    }

    if (renderInfo.isVisible)
    {
        handle_keyboard_controls(transportRect);
    }

    return createdComment;
}

auto video_element::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
{
    const float padding         = gluten::theme::space08 * 2.0f;
    const float width           = std::max(parentSize.x - padding, 0.0f);
    const float extrasWidth     = comment_markers::controls_width(m_video.format_time(m_filePosition));
    const float transportHeight = gluten::video_transport::get_height(width, extrasWidth);

    return ImVec2(parentSize.x, m_video.get_element_content_size(ImVec2(width, parentSize.y)).y + transportHeight + gluten::theme::space04 + padding);
}

auto video_element::make_comment_controls(bool& createdComment) -> gluten::transport_extras
{
    return comment_markers::make_controls(m_fileId, m_video.format_time(m_filePosition), m_filePosition, [this](double time)
                                          { seek_to_position(time); }, createdComment);
}

auto video_element::get_file_play_position() const -> double
{
    return m_video.get_file_play_position();
}

auto video_element::get_file_duration() const -> double
{
    return m_video.get_file_duration();
}

auto video_element::play_file() -> void
{
    m_video.play_file();
}

auto video_element::pause_file() -> void
{
    m_video.pause_file();
}

auto video_element::prev_frame() -> void
{
    m_video.prev_frame();
}

auto video_element::next_frame() -> void
{
    m_video.next_frame();
}

auto video_element::get_is_playing() -> bool
{
    return m_video.get_is_playing();
}

auto video_element::seek_to_position(double position) -> void
{
    m_video.seek_to_position(position);
}

video_element::inner_video_element::inner_video_element(const std::filesystem::path& filePath, int64_t fileId)
    : gluten::video_element(filePath), m_fileId(fileId)
{
}

auto video_element::inner_video_element::render_timeline_markers(const ImRect& markerLane, const std::function<float(double)>& timeToX) -> void
{
    const std::vector<const comment_data*> fileComments = comment_markers::get_file_comments(m_fileId);
    if (fileComments.empty())
    {
        return;
    }

    if (const std::optional<double> seekTime = comment_markers::render_tags(markerLane, fileComments, timeToX, false, [this](double time)
                                                                            { return format_time(time); }))
    {
        seek_to_position(*seekTime);
    }
}

auto video_element::inner_video_element::render_frame_overlay(const ImRect& frame) -> void
{
    std::shared_ptr<workspace_manager> workspaceManager = gluten::app::get()->get_manager_by_class<workspace_manager>();
    if (!workspaceManager)
    {
        return;
    }

    const comment_data* const comment = comment_markers::find_comment_at(comment_markers::get_file_comments(m_fileId), m_filePosition);
    if (!comment)
    {
        return;
    }

    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    ImFont* const boldFont     = gluten::app::get()->get_font(gluten::fonts::semibold);
    ImFont* const font         = gluten::app::get()->get_font(gluten::fonts::regular);

    const std::string displayName = workspaceManager->get_user(comment->m_userId).m_displayName;
    const std::string initials    = comment_markers::initials_of(displayName);
    const std::string firstLine   = comment->m_comment.substr(0, comment->m_comment.find('\n'));
    const std::string byline      = fmt::format("{}  {}", displayName, format_time(comment->m_timeStart));

    const float bodySize    = gluten::theme::textSizeHelper;
    const float captionSize = gluten::theme::textSizeCaption;
    const float padding     = gluten::theme::space08;
    const float maxWidth    = std::max(frame.GetWidth() * 0.6f, 120.0f);

    const float initialsWidth = boldFont->CalcTextSizeA(captionSize, FLT_MAX, 0.0f, initials.c_str()).x + padding * 2.0f;
    const float textWidth     = std::min(std::max(font->CalcTextSizeA(bodySize, FLT_MAX, 0.0f, firstLine.c_str()).x, font->CalcTextSizeA(captionSize, FLT_MAX, 0.0f, byline.c_str()).x), maxWidth - initialsWidth - padding * 2.0f);
    const float height        = bodySize + captionSize + gluten::theme::space04 * 3.0f;

    const ImVec2 min(frame.Min.x + padding, frame.Max.y - padding - height);
    const ImRect box(min, ImVec2(min.x + initialsWidth + textWidth + padding * 2.0f, min.y + height));
    const ImRect badge(box.Min, ImVec2(box.Min.x + initialsWidth, box.Max.y));

    drawList->AddRectFilled(box.Min, box.Max, ImGui::GetColorU32(gluten::theme::adjust_alpha(gluten::theme::background, 0.86f)), 3.0f);
    drawList->AddRectFilled(badge.Min, badge.Max, ImGui::GetColorU32(comment_markers::color()), 3.0f, ImDrawFlags_RoundCornersLeft);
    drawList->AddText(boldFont, captionSize, ImVec2(badge.Min.x + padding, std::floor(badge.GetCenter().y - captionSize * 0.5f - 1.0f)), ImGui::GetColorU32(gluten::theme::background), initials.c_str());

    const float textLeft = badge.Max.x + padding;
    drawList->PushClipRect(ImVec2(textLeft, box.Min.y), ImVec2(box.Max.x - padding, box.Max.y), true);
    drawList->AddText(font, bodySize, ImVec2(textLeft, box.Min.y + gluten::theme::space04), ImGui::GetColorU32(gluten::theme::textPrimary), firstLine.c_str());
    drawList->AddText(font, captionSize, ImVec2(textLeft, box.Min.y + gluten::theme::space04 * 2.0f + bodySize), ImGui::GetColorU32(gluten::theme::textHelper), byline.c_str());
    drawList->PopClipRect();

    if (ImGui::IsMouseHoveringRect(box.Min, box.Max))
    {
        ImGui::SetTooltip("%s", comment->m_comment.c_str());
    }
}
