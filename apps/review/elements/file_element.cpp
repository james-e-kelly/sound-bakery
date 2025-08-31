#include "file_element.h"

file_element::file_element(const gluten::anchor_preset& anchorPreset,
                                  const std::filesystem::path& filePath,
                                  const int64_t& fileId)
    : gluten::element(anchorPreset), m_filePath(filePath), m_fileId(fileId)
{
    set_element_padding(ImVec2(0.0, 16.0f));

    m_playButton.set_element_background_color(gluten::theme::carbon_g100::field01);
    m_fileBackground.set_element_background_color(gluten::theme::carbon_g100::field01);
    m_controlButtonsLayout.set_element_background_color(gluten::theme::carbon_g100::field01);

    m_playButton.set_element_border(1.0f, 0.0f);

    m_playButton.set_element_active_color(gluten::theme::carbon_g100::backgroundActive);
    m_pauseButton.set_element_active_color(gluten::theme::carbon_g100::backgroundActive);
    m_previousFrameButton.set_element_active_color(gluten::theme::carbon_g100::backgroundActive);
    m_nextFrameButton.set_element_active_color(gluten::theme::carbon_g100::backgroundActive);
    m_addCommentButton.set_element_active_color(gluten::theme::carbon_g100::layerActive01);

    m_playButton.set_element_hover_color(gluten::theme::carbon_g100::backgroundHover);
    m_pauseButton.set_element_hover_color(gluten::theme::carbon_g100::backgroundHover);
    m_previousFrameButton.set_element_hover_color(gluten::theme::carbon_g100::backgroundHover);
    m_nextFrameButton.set_element_hover_color(gluten::theme::carbon_g100::backgroundHover);
    m_addCommentButton.set_element_hover_color(gluten::theme::carbon_g100::layerHover01);

    m_filePositionText.set_element_alignment(ImVec2(-0.f, -0.5f));
    m_fileDurationText.set_element_alignment(ImVec2(-0.f, -0.5f));

    m_controlButtonsLayout.set_element_alignment(ImVec2(-0.5f, 0.0f));
    m_controlButtonsLayout.get_element_anchor().minOffset.x -= s_controlButtonsWidth + (s_buttonWidth * 3.0f);
    m_controlButtonsLayout.get_element_anchor().maxOffset.x += s_controlButtonsWidth + (s_buttonWidth * 3.0f);

    m_filePositionText.set_text("00:00");
    m_fileDurationText.set_text("00:00");
}

auto file_element::pre_render_element() -> void 
{
    ImGui::PushID(m_filePath.string().c_str());
}

auto file_element::render_element(const ImRect& elementRect) -> bool
{
    return m_fileBackground.render(elementRect);
}

auto file_element::post_render_element() -> void 
{
    ImGui::PopID();
}

auto file_element::render_controls() -> bool
{
    m_filePosition = get_file_play_position();
    m_fileDuration = get_file_duration();
    m_filePercent  = m_fileDuration > 0.0 ? m_filePosition / m_fileDuration : 0.0;

    m_filePositionText.set_text(fmt::format("{:02d}:{:02d}", static_cast<int>(m_filePosition) / 60, static_cast<int>(m_filePosition) % 60));
    m_fileDurationText.set_text(fmt::format("{:02d}:{:02d}", static_cast<int>(m_fileDuration) / 60, static_cast<int>(m_fileDuration) % 60));

    m_controlButtonsLayout.render_layout_element_pixels_horizontal(&m_filePositionText, s_buttonWidth);

    if (m_controlButtonsLayout.render_layout_element_pixels_horizontal(&m_previousFrameButton, s_buttonWidth))
    {
        prev_frame();
    }
    ImGui::SetItemTooltip("Previous Frame");

    if (m_controlButtonsLayout.render_layout_element_pixels_horizontal(&m_pauseButton, s_buttonWidth))
    {
        pause_file();
    }
    ImGui::SetItemTooltip("Pause");

    if (m_controlButtonsLayout.render_layout_element_pixels_horizontal(&m_playButton, s_buttonWidth))
    {
        play_file();
    }
    ImGui::SetItemTooltip("Play");

    const bool newComment = m_controlButtonsLayout.render_layout_element_pixels_horizontal(&m_addCommentButton, s_buttonWidth);
    ImGui::SetItemTooltip("Add Comment At Time");

    if (newComment)
    {
        pause_file();
    }

    if (m_controlButtonsLayout.render_layout_element_pixels_horizontal(&m_nextFrameButton, s_buttonWidth))
    {
        next_frame();
    }
    ImGui::SetItemTooltip("Next Frame");

    m_controlButtonsLayout.render_layout_element_pixels_horizontal(&m_fileDurationText, s_buttonWidth);

    return newComment;
}