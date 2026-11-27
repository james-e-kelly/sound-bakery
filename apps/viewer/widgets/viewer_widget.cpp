#include "viewer_widget.h"

#include "IconsFontaudio.h"
#include "IconsLucide.h"
#include "gluten/elements/audio_controls.h"
#include "gluten/elements/audio_element.h"
#include "gluten/subsystems/audio_subsystem.h"
#include "gluten/utils/imgui_util_structures.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include "platform/audio_file_types.h"
#include "platform/shell_integration.h"

namespace
{
    constexpr float s_padding         = gluten::theme::space08;
    constexpr float s_headerHeight    = 26.0f;
    constexpr float s_pathBarHeight   = 32.0f;
    constexpr float s_statusBarHeight = 24.0f;
    constexpr float s_rowHeight       = 24.0f;
    constexpr float s_buttonSize      = 28.0f;

    constexpr float s_minWaveformHeight     = 160.0f;
    constexpr float s_maxWaveformHeight     = 520.0f;
    constexpr float s_waveformHeightPercent = 0.4f;

    constexpr double s_typeAheadResetSeconds = 1.0;
    constexpr double s_statusMessageSeconds  = 8.0;

    constexpr const char* s_audioFileDialogFilter = "wav,wave,bwf,amb,w64,rf64,flac,mp3,ogg,oga,opus";

    auto get_audio_subsystem() -> std::shared_ptr<gluten::audio_subsystem>
    {
        return gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>();
    }

    auto format_size(std::uint64_t bytes) -> std::string
    {
        constexpr const char* s_units[] = {"B", "KB", "MB", "GB", "TB"};

        double size      = static_cast<double>(bytes);
        std::size_t unit = 0;
        while (size >= 1024.0 && unit + 1 < std::size(s_units))
        {
            size /= 1024.0;
            ++unit;
        }
        return unit == 0 ? fmt::format("{} {}", bytes, s_units[unit]) : fmt::format("{:.1f} {}", size, s_units[unit]);
    }

    auto draw_text_clipped(ImDrawList* drawList, ImFont* font, float fontSize, const ImRect& rect, const ImVec4& color, std::string_view text, float alignX = 0.0f) -> void
    {
        const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text.data(), text.data() + text.size());
        const float x         = std::floor(rect.Min.x + std::max(rect.GetWidth() - textSize.x, 0.0f) * alignX);
        const float y         = std::floor(rect.GetCenter().y - textSize.y * 0.5f);

        drawList->PushClipRect(rect.Min, rect.Max, true);
        drawList->AddText(font, fontSize, ImVec2(x, y), ImGui::GetColorU32(color), text.data(), text.data() + text.size());
        drawList->PopClipRect();
    }

    auto item_icon(const directory_item& item) -> const char*
    {
        switch (item.rootKind)
        {
            case root_kind::home:
                return ICON_LC_HOUSE;
            case root_kind::desktop:
                return ICON_LC_MONITOR;
            case root_kind::documents:
                return ICON_LC_FILE_TEXT;
            case root_kind::downloads:
                return ICON_LC_DOWNLOAD;
            case root_kind::music:
                return ICON_LC_MUSIC;
            case root_kind::videos:
                return ICON_LC_FILM;
            case root_kind::local_drive:
                return ICON_LC_HARD_DRIVE;
            case root_kind::network_drive:
                return ICON_LC_SERVER;
            case root_kind::removable_drive:
                return ICON_LC_USB;
            case root_kind::optical_drive:
                return ICON_LC_DISC;
            case root_kind::none:
                break;
        }
        return item.isDirectory ? ICON_LC_FOLDER : ICON_LC_AUDIO_WAVEFORM;
    }
}  // namespace

auto viewer_widget::start_implementation() -> void
{
    ImGuiWindowClass windowClass;
    windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
    set_window_class(windowClass);

    // Arrow keys drive the file list, so ImGui's own keyboard navigation would fight it.
    set_window_flags(ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    set_background_color(gluten::theme::background);
}

auto viewer_widget::end_implementation() -> void
{
    m_waveform.reset();

    if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = get_audio_subsystem(); audioSubsystem && !m_loadedFile.empty())
    {
        audioSubsystem->release_file(m_loadedFile);
    }
}

auto viewer_widget::open_path(const std::filesystem::path& path) -> void
{
    if (path.empty())
    {
        m_browser.navigate_to({});
        return;
    }

    const std::filesystem::path normalised = std::filesystem::absolute(path).lexically_normal();

    std::error_code error;
    if (std::filesystem::is_directory(normalised, error))
    {
        m_browser.navigate_to(normalised);
    }
    else
    {
        m_browser.navigate_to(normalised.parent_path(), normalised);
    }

    m_scrollToSelection = true;
}

auto viewer_widget::render_window_implementation() -> void
{
    m_browser.tick();

    if (m_browser.consume_selection_changed())
    {
        on_selection_changed();
    }

    gluten::imgui::scoped_style noGaps(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    const ImRect window(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
    const ImRect content(window.Min + ImVec2(s_padding, s_padding), window.Max - ImVec2(s_padding, s_padding));

    float y = content.Min.y;

    const auto take_rows = [&](float height) -> ImRect
    {
        const ImRect rect(ImVec2(content.Min.x, y), ImVec2(content.Max.x, y + height));
        y += height + gluten::theme::space04;
        return rect;
    };

    const float waveformHeight = std::clamp(content.GetHeight() * s_waveformHeightPercent, s_minWaveformHeight, s_maxWaveformHeight);

    render_file_header(take_rows(s_headerHeight));

    if (m_waveform)
    {
        gluten::audio_transport::render(take_rows(gluten::audio_transport::get_height(content.GetWidth())), *m_waveform);

        if (const float routingHeight = gluten::audio_routing_controls::get_height(m_loadedFile, content.GetWidth()); routingHeight > 0.0f)
        {
            gluten::audio_routing_controls::render(take_rows(routingHeight), m_loadedFile);
        }
    }

    const ImRect waveformRect = take_rows(waveformHeight);
    if (m_waveform)
    {
        m_waveform->render(waveformRect);
    }
    else
    {
        ImDrawList* const drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(waveformRect.Min, waveformRect.Max, ImGui::GetColorU32(gluten::theme::layer01), gluten::theme::radiusSm);
        draw_text_clipped(drawList, gluten::app::get()->get_font(gluten::fonts::regular), gluten::theme::textSizeBody, waveformRect, gluten::theme::textHelper, "Select an audio file", 0.5f);
    }

    y += gluten::theme::space04;
    render_path_bar(take_rows(s_pathBarHeight));

    const ImRect statusBar(ImVec2(content.Min.x, content.Max.y - s_statusBarHeight), content.Max);
    const ImRect listRect(ImVec2(content.Min.x, y), ImVec2(content.Max.x, std::max(y, statusBar.Min.y - gluten::theme::space04)));

    render_file_list(listRect);
    render_status_bar(statusBar);

    handle_keyboard();
}

auto viewer_widget::render_file_header(const ImRect& bar) -> void
{
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    ImFont* const titleFont    = gluten::app::get()->get_font(gluten::fonts::semibold);
    ImFont* const textFont     = gluten::app::get()->get_font(gluten::fonts::regular);

    if (m_loadedFile.empty())
    {
        draw_text_clipped(drawList, titleFont, gluten::theme::textSizeSubtitle, bar, gluten::theme::textHelper, "No file loaded");
        return;
    }

    std::string details;
    if (m_waveform && m_waveform->get_channel_count() > 0)
    {
        const std::size_t channels   = m_waveform->get_channel_count();
        const std::size_t sampleRate = m_waveform->get_sample_rate();
        if (sampleRate > 0)
        {
            details = fmt::format("{} ch  ·  {:g} kHz  ·  {}", channels, static_cast<double>(sampleRate) / 1000.0, gluten::format_timecode(m_waveform->get_duration()));
        }
        else
        {
            details = fmt::format("{} ch  ·  {}", channels, gluten::format_timecode(m_waveform->get_duration()));
        }
    }

    const float detailsWidth = textFont->CalcTextSizeA(gluten::theme::textSizeHelper, FLT_MAX, 0.0f, details.c_str()).x;
    const ImRect nameRect(bar.Min, ImVec2(bar.Max.x - detailsWidth - gluten::theme::space08, bar.Max.y));

    draw_text_clipped(drawList, titleFont, gluten::theme::textSizeSubtitle, nameRect, gluten::theme::textPrimary, path_to_utf8(m_loadedFile.filename()));
    draw_text_clipped(drawList, textFont, gluten::theme::textSizeHelper, bar, gluten::theme::textSecondary, details, 1.0f);

    ImGui::SetCursorScreenPos(nameRect.Min);
    ImGui::InvisibleButton("##FileName", nameRect.GetSize());
    ImGui::SetItemTooltip("%s", path_to_utf8(m_loadedFile).c_str());
}

auto viewer_widget::render_path_bar(const ImRect& bar) -> void
{
    const float top = std::floor(bar.Min.y + (bar.GetHeight() - s_buttonSize) * 0.5f);
    float x         = bar.Min.x;

    const auto next_button = [&]() -> ImRect
    {
        const ImRect rect(x, top, x + s_buttonSize, top + s_buttonSize);
        x += s_buttonSize + 2.0f;
        return rect;
    };

    ImFont* const iconFont   = gluten::app::get()->get_font(gluten::fonts::regular_lucide_icons);
    const auto path_button = [&](const char* id, const char* icon, bool enabled)
    {
        return gluten::transport_button(id, next_button(), icon, iconFont, gluten::theme::iconSizeSm, false, ImDrawFlags_RoundCornersAll, enabled);
    };

    if (path_button("##Back", ICON_LC_ARROW_LEFT, m_browser.can_go_back()))
    {
        m_browser.go_back();
        m_scrollToSelection = true;
    }
    ImGui::SetItemTooltip("Back (Alt+Left)");

    if (path_button("##Forward", ICON_LC_ARROW_RIGHT, m_browser.can_go_forward()))
    {
        m_browser.go_forward();
        m_scrollToSelection = true;
    }
    ImGui::SetItemTooltip("Forward (Alt+Right)");

    if (path_button("##Up", ICON_LC_ARROW_UP, m_browser.can_go_up()))
    {
        m_browser.go_up();
        m_scrollToSelection = true;
    }
    ImGui::SetItemTooltip("Up a folder (Backspace)");

    if (path_button("##Refresh", ICON_LC_ROTATE_CW, true))
    {
        m_browser.refresh();
    }
    ImGui::SetItemTooltip("Refresh (F5)");

    x += gluten::theme::space04;

    const ImGuiID pathInputId = ImGui::GetID("##Path");
    const bool editingPath    = ImGui::GetActiveID() == pathInputId;
    if (!editingPath)
    {
        m_pathText = path_to_utf8(m_browser.get_location());
    }

    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_L))
    {
        ImGui::ActivateItemByID(pathInputId);
    }

    const float frameHeight = ImGui::GetFrameHeight();
    ImGui::SetCursorScreenPos(ImVec2(x, std::floor(bar.GetCenter().y - frameHeight * 0.5f)));
    ImGui::SetNextItemWidth(bar.Max.x - x);
    ImGui::InputTextWithHint("##Path", "This PC", &m_pathText, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

    if (ImGui::IsItemDeactivatedAfterEdit() || (ImGui::IsItemDeactivated() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
    {
        const std::string trimmed = [&]
        {
            std::string text = m_pathText;
            text.erase(0, text.find_first_not_of(" \t\""));
            text.erase(text.find_last_not_of(" \t\"") + 1);
            return text;
        }();

        if (trimmed.empty())
        {
            m_browser.navigate_to({});
        }
        else
        {
            open_path(utf8_to_path(trimmed));
        }
    }
    ImGui::SetItemTooltip("Type or paste a folder, file or \\\\server\\share path (Ctrl+L)");
}

auto viewer_widget::render_file_list(const ImRect& area) -> void
{
    const std::vector<directory_item>& items = m_browser.get_items();

    ImGui::SetCursorScreenPos(area.Min);

    gluten::imgui::scoped_color childBackground(ImGuiCol_ChildBg, gluten::theme::layer01);
    gluten::imgui::scoped_style childRounding(ImGuiStyleVar_ChildRounding, gluten::theme::radiusSm);

    if (ImGui::BeginChild("##FileList", area.GetSize(), ImGuiChildFlags_None, ImGuiWindowFlags_NoNavInputs))
    {
        ImDrawList* const drawList = ImGui::GetWindowDrawList();
        ImFont* const textFont     = gluten::app::get()->get_font(gluten::fonts::regular);
        ImFont* const iconFont     = gluten::app::get()->get_font(gluten::fonts::regular_lucide_icons);

        const std::optional<std::size_t> selectedIndex = m_browser.get_selected_index();
        const float rowWidth                           = ImGui::GetContentRegionAvail().x;

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(items.size()), s_rowHeight);
        if (m_scrollToSelection && selectedIndex.has_value())
        {
            clipper.IncludeItemByIndex(static_cast<int>(*selectedIndex));
        }

        std::optional<std::size_t> clickedIndex;
        std::optional<std::size_t> doubleClickedIndex;

        while (clipper.Step())
        {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
            {
                const directory_item& item = items[row];
                const bool selected        = selectedIndex == static_cast<std::size_t>(row);

                ImGui::PushID(row);
                const ImVec2 rowMin = ImGui::GetCursorScreenPos();
                const ImRect rowRect(rowMin, rowMin + ImVec2(rowWidth, s_rowHeight));

                if (ImGui::InvisibleButton("##Row", rowRect.GetSize(), ImGuiButtonFlags_PressedOnClick))
                {
                    clickedIndex = row;
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    doubleClickedIndex = row;
                }
                const bool hovered = ImGui::IsItemHovered();

                if (!item.isDirectory && hovered)
                {
                    ImGui::SetItemTooltip("%s\n%s", format_size(item.sizeBytes).c_str(), describe_storage(item).c_str());
                }

                if (selected || hovered)
                {
                    drawList->AddRectFilled(rowRect.Min, rowRect.Max, ImGui::GetColorU32(selected ? gluten::theme::layerSelected01 : gluten::theme::layerHover01), gluten::theme::radiusSm);
                }
                if (selected && m_loadedFile == item.path)
                {
                    drawList->AddRectFilled(rowRect.Min, ImVec2(rowRect.Min.x + 3.0f, rowRect.Max.y), ImGui::GetColorU32(gluten::theme::interactive), gluten::theme::radiusSm, ImDrawFlags_RoundCornersLeft);
                }

                const ImRect iconRect(ImVec2(rowRect.Min.x + gluten::theme::space08, rowRect.Min.y), ImVec2(rowRect.Min.x + gluten::theme::space08 + 20.0f, rowRect.Max.y));
                draw_text_clipped(drawList, iconFont, gluten::theme::iconSizeSm, iconRect, item.isDirectory ? gluten::theme::textSecondary : gluten::theme::textHelper, item_icon(item));

                std::string sizeText = item.isDirectory ? std::string() : format_size(item.sizeBytes);
                const float sizeWidth = textFont->CalcTextSizeA(gluten::theme::textSizeHelper, FLT_MAX, 0.0f, sizeText.c_str()).x;
                const ImRect sizeRect(ImVec2(rowRect.Max.x - sizeWidth - gluten::theme::space08, rowRect.Min.y), ImVec2(rowRect.Max.x - gluten::theme::space08, rowRect.Max.y));
                const ImRect nameRect(ImVec2(iconRect.Max.x + gluten::theme::space04, rowRect.Min.y), ImVec2(sizeRect.Min.x - gluten::theme::space08, rowRect.Max.y));

                draw_text_clipped(drawList, textFont, gluten::theme::textSizeBody, nameRect, gluten::theme::textPrimary, item.name);
                draw_text_clipped(drawList, textFont, gluten::theme::textSizeHelper, sizeRect, gluten::theme::textHelper, sizeText);

                if (selected && m_scrollToSelection)
                {
                    const ImRect visible(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize());
                    if (rowRect.Min.y < visible.Min.y || rowRect.Max.y > visible.Max.y)
                    {
                        ImGui::SetScrollHereY(0.5f);
                    }
                    m_scrollToSelection = false;
                }

                ImGui::PopID();
            }
        }

        if (!selectedIndex.has_value())
        {
            m_scrollToSelection = false;
        }

        if (items.empty())
        {
            const ImRect emptyRect(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImVec2(ImGui::GetWindowWidth(), s_rowHeight * 3.0f));
            const char* message = m_browser.is_loading() ? "Loading..." : (m_browser.get_error().empty() ? "No folders or audio files here" : "Couldn't open this folder");
            draw_text_clipped(drawList, textFont, gluten::theme::textSizeBody, emptyRect, gluten::theme::textHelper, message, 0.5f);
        }

        if (clickedIndex.has_value())
        {
            ImGui::SetWindowFocus();
            m_browser.set_selected_index(clickedIndex);
        }
        if (doubleClickedIndex.has_value())
        {
            activate_selected();
        }
    }
    ImGui::EndChild();
}

auto viewer_widget::render_status_bar(const ImRect& bar) -> void
{
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    ImFont* const textFont     = gluten::app::get()->get_font(gluten::fonts::regular);

    if (!m_statusMessage.empty() && ImGui::GetTime() - m_statusMessageTime < s_statusMessageSeconds)
    {
        draw_text_clipped(drawList, textFont, gluten::theme::textSizeHelper, bar, m_statusMessageIsError ? gluten::theme::supportError : gluten::theme::textSecondary, m_statusMessage);
        return;
    }

    if (!m_browser.get_error().empty())
    {
        draw_text_clipped(drawList, textFont, gluten::theme::textSizeHelper, bar, gluten::theme::supportError, m_browser.get_error());
        return;
    }

    std::size_t folders = 0;
    std::size_t files   = 0;
    for (const directory_item& item : m_browser.get_items())
    {
        item.isDirectory ? ++folders : ++files;
    }

    std::string summary = m_browser.is_loading() ? std::string("Loading...") : fmt::format("{} folders  ·  {} audio files", folders, files);

    if (const directory_item* const selected = m_browser.get_selected_item(); selected && !selected->isDirectory)
    {
        summary += fmt::format("  ·  {}", describe_storage(*selected));
    }

    draw_text_clipped(drawList, textFont, gluten::theme::textSizeHelper, bar, gluten::theme::textHelper, summary);
}

auto viewer_widget::render_menu_implementation() -> void
{
    if (ImGui::BeginMenu(s_fileMenuName))
    {
        if (ImGui::MenuItem("Open Folder..."))
        {
            if (const std::filesystem::path folder = gluten::app::open_select_folder_dialog(); !folder.empty())
            {
                open_path(folder);
            }
        }

        if (ImGui::MenuItem("Open File..."))
        {
            if (const std::filesystem::path file = gluten::app::open_select_file_dialog("Audio", s_audioFileDialogFilter); !file.empty())
            {
                open_path(file);
            }
        }

        ImGui::Separator();

        const shell_integration::registration_state registration = shell_integration::get_registration_state();

        const char* registerLabel = registration == shell_integration::registration_state::registered_to_other_copy ? "Point \"Open with\" at This Copy" : "Add to Explorer's \"Open with\"";
        if (ImGui::MenuItem(registerLabel, nullptr, false, registration != shell_integration::registration_state::registered))
        {
            const tl::expected<void, std::string> result = shell_integration::register_file_types();
            set_status_message(result ? std::string("Added to \"Open with\" for audio files") : result.error(), !result.has_value());
        }

        if (ImGui::MenuItem("Make Default for Audio Files...", nullptr, false, registration != shell_integration::registration_state::not_registered))
        {
            shell_integration::open_default_apps_settings();
        }

        if (ImGui::MenuItem("Remove from Explorer's \"Open with\"", nullptr, false, registration != shell_integration::registration_state::not_registered))
        {
            const tl::expected<void, std::string> result = shell_integration::unregister_file_types();
            set_status_message(result ? std::string("Removed from \"Open with\"") : result.error(), !result.has_value());
        }

        ImGui::EndMenu();
    }
}

auto viewer_widget::handle_keyboard() -> void
{
    if (ImGui::GetIO().WantTextInput || !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        return;
    }

    const bool alt = ImGui::GetIO().KeyAlt;

    if (alt && ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false))
    {
        m_browser.go_back();
        m_scrollToSelection = true;
    }
    else if (alt && ImGui::IsKeyPressed(ImGuiKey_RightArrow, false))
    {
        m_browser.go_forward();
        m_scrollToSelection = true;
    }
    else if ((alt && ImGui::IsKeyPressed(ImGuiKey_UpArrow, false)) || ImGui::IsKeyPressed(ImGuiKey_Backspace))
    {
        m_browser.go_up();
        m_scrollToSelection = true;
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
    {
        move_selection(-1);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
    {
        move_selection(1);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_PageUp))
    {
        move_selection(-10);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_PageDown))
    {
        move_selection(10);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_Home, false))
    {
        move_selection(-static_cast<int>(m_browser.get_items().size()));
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_End, false))
    {
        move_selection(static_cast<int>(m_browser.get_items().size()));
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))
    {
        activate_selected();
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_F5, false))
    {
        m_browser.refresh();
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && m_waveform)
    {
        m_waveform->seek(std::max(m_waveform->get_position() - 1.0, 0.0));
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && m_waveform)
    {
        m_waveform->seek(std::min(m_waveform->get_position() + 1.0, m_waveform->get_duration()));
    }

    if (ImGui::IsMouseClicked(3))
    {
        m_browser.go_back();
        m_scrollToSelection = true;
    }
    else if (ImGui::IsMouseClicked(4))
    {
        m_browser.go_forward();
        m_scrollToSelection = true;
    }

    const bool typingAhead = !m_typeAheadText.empty() && ImGui::GetTime() - m_lastTypeAheadTime < s_typeAheadResetSeconds;

    for (const ImWchar character : ImGui::GetIO().InputQueueCharacters)
    {
        if (character == ' ' && !typingAhead)
        {
            toggle_playback();
        }
        else if (character > ' ' && character < 0x7F && !ImGui::GetIO().KeyCtrl && !alt)
        {
            type_ahead(static_cast<char>(character));
        }
    }
}

auto viewer_widget::move_selection(int delta) -> void
{
    const std::vector<directory_item>& items = m_browser.get_items();
    if (items.empty())
    {
        return;
    }

    const std::optional<std::size_t> selected = m_browser.get_selected_index();
    const int current                         = selected.has_value() ? static_cast<int>(*selected) : (delta > 0 ? -1 : static_cast<int>(items.size()));
    const int next                            = std::clamp(current + delta, 0, static_cast<int>(items.size()) - 1);

    m_browser.set_selected_index(static_cast<std::size_t>(next));
    m_scrollToSelection = true;
}

auto viewer_widget::type_ahead(char character) -> void
{
    const double now = ImGui::GetTime();
    if (now - m_lastTypeAheadTime > s_typeAheadResetSeconds)
    {
        m_typeAheadText.clear();
    }
    m_lastTypeAheadTime = now;
    m_typeAheadText += static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

    const std::vector<directory_item>& items = m_browser.get_items();
    const auto matches                       = [this](const directory_item& item)
    {
        if (item.name.size() < m_typeAheadText.size())
        {
            return false;
        }
        for (std::size_t index = 0; index < m_typeAheadText.size(); ++index)
        {
            if (std::tolower(static_cast<unsigned char>(item.name[index])) != m_typeAheadText[index])
            {
                return false;
            }
        }
        return true;
    };

    if (const auto found = std::find_if(items.begin(), items.end(), matches); found != items.end())
    {
        m_browser.set_selected_index(static_cast<std::size_t>(std::distance(items.begin(), found)));
        m_scrollToSelection = true;
    }
}

auto viewer_widget::on_selection_changed() -> void
{
    const directory_item* const selected = m_browser.get_selected_item();
    if (!selected || selected->isDirectory || selected->path == m_loadedFile)
    {
        return;
    }

    get_app()->get_logger()->info("Selected '{}' ({}, {})", path_to_utf8(selected->path), format_size(selected->sizeBytes), describe_storage(*selected));

    load_file(selected->path);

    if (m_waveform)
    {
        m_waveform->play();
    }
}

auto viewer_widget::load_file(const std::filesystem::path& file) -> void
{
    std::shared_ptr<gluten::audio_subsystem> audioSubsystem = get_audio_subsystem();
    if (!audioSubsystem)
    {
        return;
    }

    m_waveform.reset();

    if (!m_loadedFile.empty())
    {
        audioSubsystem->release_file(m_loadedFile);
    }

    m_loadedFile = file;
    m_waveform   = std::make_unique<gluten::audio_element>(file);

    if (!audioSubsystem->get_or_load_audio_handle(file))
    {
        get_app()->get_logger()->error("Couldn't open '{}' for playback", path_to_utf8(file));
        set_status_message("Couldn't open this file for playback", true);
    }
}

auto viewer_widget::activate_selected() -> void
{
    const directory_item* const selected = m_browser.get_selected_item();
    if (!selected)
    {
        return;
    }

    if (selected->isDirectory)
    {
        m_browser.navigate_to(selected->path);
        m_scrollToSelection = true;
        return;
    }

    if (selected->path != m_loadedFile)
    {
        load_file(selected->path);
    }

    if (m_waveform)
    {
        m_waveform->seek(0.0);
        m_waveform->play();
    }
}

auto viewer_widget::toggle_playback() -> void
{
    if (!m_waveform)
    {
        activate_selected();
        return;
    }

    m_waveform->is_playing() ? m_waveform->pause() : m_waveform->play();
}

auto viewer_widget::set_status_message(std::string message, bool isError) -> void
{
    m_statusMessage        = std::move(message);
    m_statusMessageIsError = isError;
    m_statusMessageTime    = ImGui::GetTime();
}
