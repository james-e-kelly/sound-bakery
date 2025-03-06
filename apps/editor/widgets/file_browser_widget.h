#pragma once

#include "gluten/widgets/widget.h"

#include <cstdint>

enum class ButtonState
{
    NONE = 0,
    HOVERED,
    CLICKED
};

class file_browser_widget : public gluten::widget
{
    WIDGET_CONSTRUCT(file_browser_widget, "File Browser Widget")

public:
    virtual void start_implementation() override;
    virtual void render_implementation() override;

    const std::string& get_selected_filename() const { return m_selectedFileString; }

private:
    void unselect_item() noexcept;
    ButtonState draw_wide_button(bool selected, uint32_t hoveredColor, uint32_t activeColor) const noexcept;
    void show_item_context_menu(const std::filesystem::path& path) const noexcept;
    void show_nav_menu() noexcept;
    void show_directory_browser_list() noexcept;

private:
    std::string m_selectedFile;
    std::string m_draggingFile;
    std::filesystem::path m_currentDirectory;
    std::filesystem::path m_topDir;  // cannot go higher than this dir
    uint32_t m_selectedItemID = 0;
    std::string m_selectedFileString;
};
