#pragma once

#include "browser/file_browser.h"
#include "gluten/elements/audio_element.h"
#include "gluten/widgets/window_widget.h"

/**
 * @brief The whole Sound Viewer window: the selected file's waveform and transport on top, a file browser underneath.
 */
class viewer_widget : public gluten::window_widget
{
    WIDGET_CONSTRUCT_PARENT(viewer_widget, "Viewer", gluten::window_widget)

public:
    /**
     * @brief Browses to a folder, or to a file's folder with that file selected and playing.
     */
    auto open_path(const std::filesystem::path& path) -> void;

protected:
    auto start_implementation() -> void override;
    auto render_window_implementation() -> void override;
    auto render_menu_implementation() -> void override;
    auto end_implementation() -> void override;

private:
    auto render_file_header(const ImRect& bar) -> void;
    auto render_path_bar(const ImRect& bar) -> void;
    auto render_file_list(const ImRect& area) -> void;
    auto render_status_bar(const ImRect& bar) -> void;
    auto handle_keyboard() -> void;

    auto on_selection_changed() -> void;
    auto load_file(const std::filesystem::path& file) -> void;
    auto activate_selected() -> void;
    auto toggle_playback() -> void;
    auto move_selection(int delta) -> void;
    auto type_ahead(char character) -> void;
    auto set_status_message(std::string message, bool isError) -> void;

    file_browser m_browser;

    std::unique_ptr<gluten::audio_element> m_waveform;
    std::filesystem::path m_loadedFile;

    std::string m_pathText;
    bool m_scrollToSelection = false;

    std::string m_typeAheadText;
    double m_lastTypeAheadTime = 0.0;

    std::string m_statusMessage;
    bool m_statusMessageIsError = false;
    double m_statusMessageTime  = 0.0;
};
