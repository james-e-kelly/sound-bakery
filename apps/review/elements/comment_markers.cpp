#include "comment_markers.h"

#include "IconsLucide.h"
#include "gluten/app/app.h"
#include "gluten/elements/audio_element.h"
#include "managers/workspace_manager.h"

namespace
{
    constexpr double s_adjacentCommentGap = 0.05;
    constexpr double s_pointCommentHold   = 2.0;  //< How long a comment without an end stays "at" the playhead
    constexpr float s_navWidth            = 28.0f;
    constexpr float s_maxTagWidth         = 220.0f;
    constexpr float s_flagWidth           = 5.0f;

    auto button_label(const std::string& positionLabel) -> std::string
    {
        return fmt::format(ICON_LC_MESSAGE_SQUARE_PLUS "  Comment at {}", positionLabel);
    }

    auto button_width(const std::string& label) -> float
    {
        ImFont* const font = gluten::app::get()->get_font(gluten::fonts::regular_lucide_icons);
        return std::ceil(font->CalcTextSizeA(gluten::theme::textSizeHelper, FLT_MAX, 0.0f, label.c_str()).x) + gluten::theme::space08 * 2.0f + 4.0f;
    }
}  // namespace

namespace comment_markers
{
    auto color() -> ImVec4 { return gluten::theme::hex_to_imgui_imvec4(0x08bdba); }

    auto initials_of(const std::string& displayName) -> std::string
    {
        std::string initials;
        bool atWordStart = true;
        for (const char& character : displayName)
        {
            if (std::isspace(static_cast<unsigned char>(character)))
            {
                atWordStart = true;
            }
            else if (atWordStart && initials.size() < 2)
            {
                initials += static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
                atWordStart = false;
            }
            else
            {
                atWordStart = false;
            }
        }
        return initials.empty() ? std::string("?") : initials;
    }

    auto get_file_comments(int64_t fileId) -> std::vector<const comment_data*>
    {
        std::vector<const comment_data*> fileComments;

        if (std::shared_ptr<workspace_manager> workspaceManager = gluten::app::get()->get_manager_by_class<workspace_manager>())
        {
            const auto& comments = workspaceManager->get_all_comments_for_review(workspaceManager->get_selected_review().m_reviewId);

            if (comments.has_data())
            {
                for (const auto& comment : comments.m_cache)
                {
                    if (comment.m_fileId == fileId && comment.m_timeStart >= 0.0)
                    {
                        fileComments.push_back(&comment);
                    }
                }
            }
        }

        std::sort(fileComments.begin(), fileComments.end(), [](const comment_data* a, const comment_data* b)
                  { return a->m_timeStart < b->m_timeStart; });
        return fileComments;
    }

    auto find_comment_at(const std::vector<const comment_data*>& comments, double time) -> const comment_data*
    {
        for (auto comment = comments.rbegin(); comment != comments.rend(); ++comment)
        {
            const double end = (*comment)->m_timeEnd > (*comment)->m_timeStart ? (*comment)->m_timeEnd : (*comment)->m_timeStart + s_pointCommentHold;
            if (time >= (*comment)->m_timeStart && time < end)
            {
                return *comment;
            }
        }
        return nullptr;
    }

    auto render_tags(const ImRect& markerLane, const std::vector<const comment_data*>& comments, const std::function<float(double)>& timeToX, bool compact, const std::function<std::string(double)>& formatTime) -> std::optional<double>
    {
        std::shared_ptr<workspace_manager> workspaceManager = gluten::app::get()->get_manager_by_class<workspace_manager>();
        if (!workspaceManager)
        {
            return std::nullopt;
        }

        const ImU32 tagColor     = ImGui::GetColorU32(color());
        const ImU32 tagTextColor = ImGui::GetColorU32(gluten::theme::background);
        const float labelSize    = gluten::theme::textSizeCaption;

        ImFont* const font         = gluten::app::get()->get_font(gluten::fonts::semibold);
        ImDrawList* const drawList = ImGui::GetWindowDrawList();

        const auto time_to_x = [&timeToX](double time)
        { return std::floor(timeToX(time)) + 0.5f; };

        std::optional<double> seekTime;

        drawList->PushClipRect(markerLane.Min, markerLane.Max, true);
        for (std::size_t index = 0; index < comments.size(); ++index)
        {
            const comment_data* comment = comments[index];
            const float x               = time_to_x(comment->m_timeStart);

            if (comment->m_timeEnd > comment->m_timeStart)
            {
                drawList->AddRectFilled(ImVec2(x, markerLane.Max.y - 3.0f), ImVec2(time_to_x(comment->m_timeEnd), markerLane.Max.y - 1.0f), tagColor);
            }
            drawList->AddLine(ImVec2(x, markerLane.Min.y + 3.0f), ImVec2(x, markerLane.Max.y), tagColor);

            const std::string displayName = workspaceManager->get_user(comment->m_userId).m_displayName;
            const std::string initials    = initials_of(displayName);
            const std::string firstLine   = comment->m_comment.substr(0, comment->m_comment.find('\n'));
            const std::string label       = compact ? initials : fmt::format("{}  {}", initials, firstLine);

            const float nextX         = index + 1 < comments.size() ? time_to_x(comments[index + 1]->m_timeStart) - 2.0f : markerLane.Max.x;
            const float initialsWidth = font->CalcTextSizeA(labelSize, FLT_MAX, 0.0f, initials.c_str()).x + 10.0f;
            const float labelWidth    = font->CalcTextSizeA(labelSize, FLT_MAX, 0.0f, label.c_str()).x + 10.0f;
            const float roomToNext    = nextX - x;
            const float tagWidth      = roomToNext >= initialsWidth ? std::max(std::min({labelWidth, roomToNext, s_maxTagWidth}), initialsWidth) : std::max(roomToNext, s_flagWidth);

            const ImRect tag(ImVec2(x, markerLane.Min.y + 3.0f), ImVec2(x + tagWidth, markerLane.Max.y - 5.0f));
            drawList->AddRectFilled(tag.Min, tag.Max, tagColor, 2.0f, ImDrawFlags_RoundCornersRight);

            drawList->PushClipRect(tag.Min, ImVec2(tag.Max.x - 3.0f, tag.Max.y), true);
            drawList->AddText(font, labelSize, ImVec2(tag.Min.x + 5.0f, std::floor(tag.GetCenter().y - labelSize * 0.5f - 1.0f)), tagTextColor, label.c_str());
            drawList->PopClipRect();

            if (ImGui::IsMouseHoveringRect(tag.Min, tag.Max))
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                ImGui::SetTooltip("%s at %s\n%s", displayName.c_str(), formatTime(comment->m_timeStart).c_str(), comment->m_comment.c_str());

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    seekTime = comment->m_timeStart;
                }
            }
        }
        drawList->PopClipRect();

        return seekTime;
    }

    auto controls_width(const std::string& positionLabel) -> float
    {
        return s_navWidth * 2.0f + button_width(button_label(positionLabel)) - 2.0f;
    }

    auto make_controls(int64_t fileId, const std::string& positionLabel, double position, std::function<void(double)> seek, bool& createdComment) -> gluten::transport_extras
    {
        gluten::transport_extras extras;
        extras.width  = controls_width(positionLabel);
        extras.render = [fileId, positionLabel, position, seek = std::move(seek), &createdComment](const ImRect& area)
        {
            ImFont* const lucideFont = gluten::app::get()->get_font(gluten::fonts::regular_lucide_icons);
            const std::string label  = button_label(positionLabel);
            const float labelWidth   = button_width(label);

            float x              = area.Min.x;
            const auto next_rect = [&x, &area](float width)
            { return ImRect(ImVec2(x, area.Min.y), ImVec2(x + width, area.Max.y)); };

            const auto seek_adjacent = [fileId, position, &seek](bool forwards)
            {
                const std::vector<const comment_data*> comments = get_file_comments(fileId);

                if (forwards)
                {
                    const auto next = std::find_if(comments.begin(), comments.end(), [position](const comment_data* comment)
                                                   { return comment->m_timeStart > position + s_adjacentCommentGap; });
                    if (next != comments.end())
                    {
                        seek((*next)->m_timeStart);
                    }
                }
                else
                {
                    const auto previous = std::find_if(comments.rbegin(), comments.rend(), [position](const comment_data* comment)
                                                       { return comment->m_timeStart < position - s_adjacentCommentGap; });
                    if (previous != comments.rend())
                    {
                        seek((*previous)->m_timeStart);
                    }
                }
            };

            if (gluten::transport_button("##PrevComment", next_rect(s_navWidth), ICON_LC_CHEVRON_LEFT, lucideFont, gluten::theme::iconSizeMd, false, ImDrawFlags_RoundCornersLeft))
            {
                seek_adjacent(false);
            }
            ImGui::SetItemTooltip("Previous comment");
            x += s_navWidth - 1.0f;

            if (gluten::transport_button("##AddComment", next_rect(labelWidth), label.c_str(), lucideFont, gluten::theme::textSizeHelper, false, ImDrawFlags_RoundCornersNone))
            {
                createdComment = true;
            }
            ImGui::SetItemTooltip("Add a comment at the playhead");
            x += labelWidth - 1.0f;

            if (gluten::transport_button("##NextComment", next_rect(s_navWidth), ICON_LC_CHEVRON_RIGHT, lucideFont, gluten::theme::iconSizeMd, false, ImDrawFlags_RoundCornersRight))
            {
                seek_adjacent(true);
            }
            ImGui::SetItemTooltip("Next comment");
        };
        return extras;
    }
}  // namespace comment_markers
