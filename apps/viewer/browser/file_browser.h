#pragma once

#include "platform/directory_listing.h"

#include "concurrencpp/concurrencpp.h"

/**
 * @brief Where the browser is, what's in it and what's selected. Listings run in the background so a slow
 * network share never stalls the UI.
 *
 * An empty location is the root view (drives on Windows).
 */
class file_browser
{
public:
    /**
     * @brief Moves to a directory, recording history. Selects selectPath once the listing arrives, if it's there.
     */
    auto navigate_to(const std::filesystem::path& directory, const std::filesystem::path& selectPath = {}) -> void;

    auto go_back() -> void;
    auto go_forward() -> void;
    auto go_up() -> void;
    auto refresh() -> void;

    auto can_go_back() const -> bool { return !m_backHistory.empty(); }
    auto can_go_forward() const -> bool { return !m_forwardHistory.empty(); }
    auto can_go_up() const -> bool { return !m_location.empty(); }

    /**
     * @brief Collects a finished listing. Call once per frame on the main thread.
     */
    auto tick() -> void;

    auto get_location() const -> const std::filesystem::path& { return m_location; }
    auto get_items() const -> const std::vector<directory_item>& { return m_items; }
    auto get_error() const -> const std::string& { return m_error; }
    auto is_loading() const -> bool { return static_cast<bool>(m_pendingListing); }

    auto get_selected_index() const -> std::optional<std::size_t> { return m_selectedIndex; }
    auto get_selected_item() const -> const directory_item*;
    auto set_selected_index(std::optional<std::size_t> index) -> void;

    /**
     * @brief Set when the selection changed since the last call. Lets the UI scroll to it or start playback.
     */
    auto consume_selection_changed() -> bool;

private:
    using listing_result = tl::expected<std::vector<directory_item>, std::string>;

    auto load(std::filesystem::path directory, std::filesystem::path selectPath) -> void;
    auto apply_listing(listing_result listing) -> void;

    std::filesystem::path m_location;
    bool m_hasNavigated = false;
    std::vector<std::filesystem::path> m_backHistory;
    std::vector<std::filesystem::path> m_forwardHistory;

    std::vector<directory_item> m_items;
    std::string m_error;

    std::optional<std::size_t> m_selectedIndex;
    std::filesystem::path m_pendingSelectPath;
    bool m_selectionChanged = false;

    concurrencpp::result<listing_result> m_pendingListing;
    std::chrono::steady_clock::time_point m_loadStartTime;
};
