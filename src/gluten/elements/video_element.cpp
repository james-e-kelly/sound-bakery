#include "video_element.h"

#include "gluten/app/app.h"
#include "gluten/subsystems/video_subsystem.h"
#include "gluten/theme/theme.h"

namespace
{
    constexpr float g_videoControlRowHeight = 30.0f;
    constexpr float g_videoControlRowHalfHeight = g_videoControlRowHeight / 2.0f;
    constexpr int g_videoControlRows           = 2;  // timeline + overlay
    constexpr float g_totalVideoControlsHeight = g_videoControlRowHeight * g_videoControlRows;

    constexpr float g_progressLineThickness    = 1.0f;
    constexpr float g_minimumVideoPosition     = 0.0;

    // Shared horizontal inset for the timeline and overlay rows, so the timeline's draggable range
    // and any markers an overlay draws line up on the same x-range.
    constexpr float g_overlayRowInset = 20.0f;
}

namespace gluten
{
    video_element::video_element(const std::filesystem::path& videoFile)
        : file_element(anchor_preset::stretch_full, videoFile)
    {
        if (m_videoSubsystem.expired())
        {
            m_videoSubsystem = app::get()->get_subsystem_by_class<video_subsystem>();
        }

        if (std::shared_ptr<video_subsystem> videoSubsystem = m_videoSubsystem.lock())
        {
            m_videoTexture = videoSubsystem->get_video_texture(m_filePath.string());

            if (m_videoTexture == 0)
            {
                videoSubsystem->load_video(m_filePath);
            }

            m_videoTexture = videoSubsystem->get_video_texture(m_filePath.string());

            m_videoImage = image(m_videoTexture, 1920, 1080);
        }

        m_videoBottomLayout.set_element_background_color(theme::background);

        m_videoOverlayLayout.set_element_padding(ImVec2(g_overlayRowInset, 0.0f));
        m_videoTimelineLayout.set_element_padding(ImVec2(g_overlayRowInset, 0.0f));
    }

    auto video_element::render_element(const element_render_info& renderInfo) -> bool
    {
        std::shared_ptr<video_subsystem> videoSubsystem = m_videoSubsystem.lock();
        if (!videoSubsystem || !renderInfo.isVisible)
        {
            return false;
        }

        m_videoImage.render(renderInfo.elementBox);

        render_layouts(renderInfo.elementBox);
        render_timeline();
        render_video_overlay();

        handle_keyboard_controls(m_videoImage.get_element_rect());
        handle_mouse_controls(m_videoImage.get_element_rect());

        return false;
    }

    auto video_element::render_timeline() -> void
    {
        const float timelineWidth = m_videoTimelineLayout.get_element_rect().GetWidth();

        ImVec2 progressLineStart = m_videoTimelineLayout.get_element_rect().GetTL();
        progressLineStart.y += g_videoControlRowHalfHeight;

        ImVec2 progressLineEnd = progressLineStart;
        progressLineEnd.x += timelineWidth;

        if (ImDrawList* const drawList = ImGui::GetWindowDrawList())
        {
            drawList->AddLine(ImVec2(progressLineStart.x, progressLineStart.y),
                              ImVec2(progressLineEnd.x, progressLineEnd.y),
                              ImGui::ColorConvertFloat4ToU32(theme::textPrimary),
                              g_progressLineThickness);

            const ImGuiID videoGrabHandleId = ImGui::GetID("##VideoDragHandle");
            ImGui::KeepAliveID(videoGrabHandleId);

            ImRect grabRect(progressLineStart.x - (ImGui::GetStyle().GrabMinSize / 2.0f),
                            progressLineStart.y - (g_videoControlRowHeight / 3.0f),
                            progressLineEnd.x + (ImGui::GetStyle().GrabMinSize / 2.0f),
                            progressLineEnd.y + (g_videoControlRowHeight / 3.0f));

            const bool hovered = ImGui::ItemHoverable(grabRect, videoGrabHandleId, ImGuiSliderFlags_NoInput);

            const bool clicked    = hovered && ImGui::IsMouseClicked(0, ImGuiInputFlags_None, videoGrabHandleId);
            const bool makeActive = (clicked || ImGui::GetCurrentContext()->NavActivateId == videoGrabHandleId);

            if (makeActive && clicked)
            {
                ImGui::SetKeyOwner(ImGuiKey_MouseLeft, videoGrabHandleId);
            }

            if (makeActive)
            {
                ImGui::SetActiveID(videoGrabHandleId, ImGui::GetCurrentWindow());
                ImGui::SetFocusID(videoGrabHandleId, ImGui::GetCurrentWindow());
                ImGui::FocusWindow(ImGui::GetCurrentWindow());
                ImGui::GetCurrentContext()->ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
            }

            ImRect outDrag;
            if (ImGui::SliderBehavior(grabRect, videoGrabHandleId, ImGuiDataType_Double, &m_filePosition,
                                      &g_minimumVideoPosition, &m_fileDuration, "%f", ImGuiSliderFlags_None, &outDrag))
            {
                ImGui::MarkItemEdited(videoGrabHandleId);
                m_videoSubsystem.lock()->set_video_play_position(m_filePath, m_filePosition);
            }

            if (outDrag.Max.x > outDrag.Min.x)
            {
                drawList->AddRectFilled(outDrag.Min, outDrag.Max,
                                        ImGui::ColorConvertFloat4ToU32(theme::interactive), 0.0f);
            }
        }
    }

    auto video_element::render_layouts(const ImRect& elementRect) -> void
    {
        ImRect videoBottomRect = elementRect;
        videoBottomRect.Min.y  = videoBottomRect.Max.y - g_totalVideoControlsHeight;
        m_videoBottomLayout.render(videoBottomRect);

        m_videoBottomLayout.render_layout_element_pixels_vertical(&m_videoTimelineLayout, g_videoControlRowHeight);
        m_videoBottomLayout.render_layout_element_pixels_vertical(&m_videoOverlayLayout, g_videoControlRowHeight);
    }

    auto video_element::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
    {
        const ImVec2 videoSize = m_videoImage.get_element_content_size(parentSize);
        return ImVec2(videoSize.x, videoSize.y + g_totalVideoControlsHeight);
    }

    auto video_element::get_file_play_position() const -> double
    {
        return m_videoSubsystem.lock()->get_video_play_position(m_filePath);
    }

    auto video_element::get_file_duration() const -> double
    {
        return m_videoSubsystem.lock()->get_video_duration(m_filePath);
    }

    auto video_element::play_file() -> void
    {
        m_videoSubsystem.lock()->play_video(m_filePath);
    }

    auto video_element::pause_file() -> void
    {
        m_videoSubsystem.lock()->pause_video(m_filePath);
    }

    auto video_element::seek_to_position(double position) -> void
    {
        m_videoSubsystem.lock()->set_video_play_position(m_filePath, position);
    }

    auto video_element::prev_frame() -> void
    {
        m_videoSubsystem.lock()->set_video_prev_frame(m_filePath);
    }

    auto video_element::next_frame() -> void
    {
        m_videoSubsystem.lock()->set_video_next_frame(m_filePath);
    }

    auto video_element::get_is_playing() -> bool
    {
        return m_videoSubsystem.lock()->get_video_is_playing(m_filePath);
    }
}  // namespace gluten
