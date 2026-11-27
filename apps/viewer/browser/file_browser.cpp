#include "file_browser.h"

#include "gluten/app/app.h"
#include "platform/audio_file_types.h"

#if defined(_WIN32)
    #include <windows.h>
#endif

namespace
{
    /**
     * @brief Case-insensitive, with digit runs compared by value so "take_2" sorts before "take_10".
     */
    auto natural_less(std::string_view left, std::string_view right) -> bool
    {
        std::size_t leftIndex  = 0;
        std::size_t rightIndex = 0;

        while (leftIndex < left.size() && rightIndex < right.size())
        {
            const unsigned char leftChar  = static_cast<unsigned char>(left[leftIndex]);
            const unsigned char rightChar = static_cast<unsigned char>(right[rightIndex]);

            if (std::isdigit(leftChar) && std::isdigit(rightChar))
            {
                const std::size_t leftStart  = leftIndex;
                const std::size_t rightStart = rightIndex;
                while (leftIndex < left.size() && std::isdigit(static_cast<unsigned char>(left[leftIndex])))
                {
                    ++leftIndex;
                }
                while (rightIndex < right.size() && std::isdigit(static_cast<unsigned char>(right[rightIndex])))
                {
                    ++rightIndex;
                }

                std::string_view leftDigits  = left.substr(leftStart, leftIndex - leftStart);
                std::string_view rightDigits = right.substr(rightStart, rightIndex - rightStart);
                while (leftDigits.size() > 1 && leftDigits.front() == '0')
                {
                    leftDigits.remove_prefix(1);
                }
                while (rightDigits.size() > 1 && rightDigits.front() == '0')
                {
                    rightDigits.remove_prefix(1);
                }

                if (leftDigits.size() != rightDigits.size())
                {
                    return leftDigits.size() < rightDigits.size();
                }
                if (leftDigits != rightDigits)
                {
                    return leftDigits < rightDigits;
                }
                continue;
            }

            const int leftLower  = std::tolower(leftChar);
            const int rightLower = std::tolower(rightChar);
            if (leftLower != rightLower)
            {
                return leftLower < rightLower;
            }

            ++leftIndex;
            ++rightIndex;
        }

        return (left.size() - leftIndex) < (right.size() - rightIndex);
    }

    auto is_unc_path(const std::filesystem::path& path) -> bool
    {
        const std::wstring rootName = path.root_name().wstring();
        return rootName.size() > 2 && (rootName[0] == L'\\' || rootName[0] == L'/') && (rootName[1] == L'\\' || rootName[1] == L'/');
    }

    /**
     * @brief The directory above, or empty for the root view. A UNC share's parent is the server, which can't be listed like a folder.
     */
    auto get_parent_location(const std::filesystem::path& location) -> std::filesystem::path
    {
        if (location.empty() || location.relative_path().empty())
        {
            return {};
        }

        const std::filesystem::path parent = location.parent_path();
        if (parent == location || (parent.relative_path().empty() && is_unc_path(parent)))
        {
            return {};
        }

        return parent;
    }
}  // namespace

auto file_browser::navigate_to(const std::filesystem::path& directory, const std::filesystem::path& selectPath) -> void
{
    if (directory == m_location && !m_items.empty() && !is_loading())
    {
        const auto found = std::find_if(m_items.begin(), m_items.end(), [&selectPath](const directory_item& item) { return item.path == selectPath; });
        if (found != m_items.end())
        {
            set_selected_index(static_cast<std::size_t>(std::distance(m_items.begin(), found)));
        }
        return;
    }

    if (m_hasNavigated)
    {
        m_backHistory.push_back(m_location);
        m_forwardHistory.clear();
    }

    m_hasNavigated = true;
    load(directory, selectPath);
}

auto file_browser::go_back() -> void
{
    if (m_backHistory.empty())
    {
        return;
    }

    m_forwardHistory.push_back(m_location);
    const std::filesystem::path previous = m_backHistory.back();
    m_backHistory.pop_back();
    load(previous, m_location);
}

auto file_browser::go_forward() -> void
{
    if (m_forwardHistory.empty())
    {
        return;
    }

    m_backHistory.push_back(m_location);
    const std::filesystem::path next = m_forwardHistory.back();
    m_forwardHistory.pop_back();
    load(next, {});
}

auto file_browser::go_up() -> void
{
    if (can_go_up())
    {
        navigate_to(get_parent_location(m_location), m_location);
    }
}

auto file_browser::refresh() -> void
{
    const directory_item* const selected = get_selected_item();
    load(m_location, selected ? selected->path : std::filesystem::path());
}

auto file_browser::load(std::filesystem::path directory, std::filesystem::path selectPath) -> void
{
    // Taken by value: callers often pass m_location itself as the path to select.
    m_location          = directory;
    m_pendingSelectPath = std::move(selectPath);
    m_error.clear();
    m_loadStartTime = std::chrono::steady_clock::now();

    m_pendingListing = gluten::app::get()->background_executor()->submit(
        [directory]() -> listing_result
        {
            if (directory.empty())
            {
                return list_roots();
            }
            return list_directory(directory);
        });
}

auto file_browser::tick() -> void
{
    if (m_pendingListing && m_pendingListing.status() != concurrencpp::result_status::idle)
    {
        listing_result listing = m_pendingListing.get();
        apply_listing(std::move(listing));
    }
}

auto file_browser::apply_listing(listing_result listing) -> void
{
    m_items.clear();
    m_selectedIndex.reset();
    m_selectionChanged = true;

    const std::string location = m_location.empty() ? std::string("drives") : path_to_utf8(m_location);
    const auto elapsed         = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_loadStartTime);

    if (!listing.has_value())
    {
        m_error = listing.error();
        gluten::app::get()->get_logger()->warn("Couldn't list '{}' after {} ms: {}", location, elapsed.count(), m_error);
        return;
    }

    const std::size_t listedCount = listing->size();
    std::size_t reparsePointCount = 0;
    std::size_t dedupCount        = 0;

    for (directory_item& item : listing.value())
    {
        if (item.reparseTag != 0)
        {
            ++reparsePointCount;
#if defined(_WIN32)
            dedupCount += item.reparseTag == IO_REPARSE_TAG_DEDUP ? 1 : 0;
#endif
        }

        if (item.isDirectory || is_supported_audio_file(item.path))
        {
            m_items.push_back(std::move(item));
        }
    }

    gluten::app::get()->get_logger()->info("Listed '{}' in {} ms: {} entries, {} kept, {} reparse points ({} Data Deduplication)", location, elapsed.count(), listedCount, m_items.size(), reparsePointCount, dedupCount);

    if (!m_location.empty())
    {
        std::sort(m_items.begin(), m_items.end(),
                  [](const directory_item& left, const directory_item& right)
                  {
                      if (left.isDirectory != right.isDirectory)
                      {
                          return left.isDirectory;
                      }
                      return natural_less(left.name, right.name);
                  });
    }

    if (!m_pendingSelectPath.empty())
    {
        const auto found = std::find_if(m_items.begin(), m_items.end(), [this](const directory_item& item) { return item.path == m_pendingSelectPath; });
        if (found != m_items.end())
        {
            m_selectedIndex = static_cast<std::size_t>(std::distance(m_items.begin(), found));
        }
        m_pendingSelectPath.clear();
    }
}

auto file_browser::get_selected_item() const -> const directory_item*
{
    return m_selectedIndex.has_value() && *m_selectedIndex < m_items.size() ? &m_items[*m_selectedIndex] : nullptr;
}

auto file_browser::set_selected_index(std::optional<std::size_t> index) -> void
{
    if (index.has_value() && *index >= m_items.size())
    {
        index.reset();
    }

    if (index != m_selectedIndex)
    {
        m_selectedIndex    = index;
        m_selectionChanged = true;
    }
}

auto file_browser::consume_selection_changed() -> bool
{
    return std::exchange(m_selectionChanged, false);
}
