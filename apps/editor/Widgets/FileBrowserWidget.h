#pragma once

#include "Widget.h"

#include <cstdint>

enum class ButtonState
{
    NONE = 0,
    HOVERED,
    CLICKED
};

class FileBrowserWidget : public widget
{
    WIDGET_CONSTRUCT(FileBrowserWidget)

public:
    virtual void start() override;
    virtual void Render() override;

    const std::string& GetSelectedFileName() const
    {
        return m_selectedFileString;
    }

private:
    void UnselectItem() noexcept;
    ButtonState DrawWideButton(bool selected,
                               uint32_t hoveredColor,
                               uint32_t activeColor) const noexcept;
    void ShowItemContextMenu(const std::filesystem::path& path) const noexcept;
    void ShowNavMenu() noexcept;
    void ShowDirectoryBrowserList() noexcept;

private:
    std::string m_selectedFile;
    std::string m_draggingFile;
    std::filesystem::path m_currentDirectory;
    std::filesystem::path m_topDir;  // cannot go higher than this dir
    uint32_t m_selectedItemID = 0;
    std::string m_selectedFileString;
};
