#include "directory_listing.h"

#include "fmt/format.h"

#if defined(_WIN32)
    #include <shlobj.h>
    #include <windows.h>
#else
    #include <cstdlib>
#endif

auto path_to_utf8(const std::filesystem::path& path) -> std::string
{
    const std::u8string utf8 = path.u8string();
    return std::string(utf8.begin(), utf8.end());
}

auto utf8_to_path(std::string_view utf8) -> std::filesystem::path
{
    return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
}

#if defined(_WIN32)

namespace
{
    /**
     * @brief Prefixes a path with \\?\ so Win32 doesn't cap it at MAX_PATH. Deep sound libraries on shares go past 260 characters.
     */
    auto to_extended_length_path(const std::filesystem::path& path) -> std::wstring
    {
        std::wstring native = std::filesystem::absolute(path).lexically_normal().make_preferred().native();

        if (native.starts_with(LR"(\\?\)"))
        {
            return native;
        }
        if (native.starts_with(LR"(\\)"))
        {
            return LR"(\\?\UNC\)" + native.substr(2);
        }
        return LR"(\\?\)" + native;
    }

    auto win32_error_message(DWORD error) -> std::string
    {
        return fmt::format("{} (error {})", std::system_category().message(static_cast<int>(error)), error);
    }

    auto reparse_tag_name(std::uint32_t tag) -> std::string
    {
        switch (tag)
        {
            case IO_REPARSE_TAG_DEDUP:
                return "Data Deduplication";
            case IO_REPARSE_TAG_SYMLINK:
                return "Symbolic link";
            case IO_REPARSE_TAG_MOUNT_POINT:
                return "Junction";
            case IO_REPARSE_TAG_WOF:
                return "Compressed (WOF)";
            case IO_REPARSE_TAG_HSM:
            case IO_REPARSE_TAG_HSM2:
                return "Hierarchical storage";
            case IO_REPARSE_TAG_DFS:
            case IO_REPARSE_TAG_DFSR:
                return "DFS";
            default:
                break;
        }

        if ((tag & 0xFFFF0FFF) == IO_REPARSE_TAG_CLOUD)
        {
            return "Cloud placeholder";
        }

        return fmt::format("Tag 0x{:08X}", tag);
    }

    /**
     * @brief Resolves through the shell rather than guessing from the profile path, so folders moved
     * elsewhere (OneDrive backup, another drive) are found where they actually live.
     */
    auto get_known_folder(REFKNOWNFOLDERID folderId) -> std::filesystem::path
    {
        PWSTR rawPath = nullptr;
        std::filesystem::path path;
        if (SUCCEEDED(SHGetKnownFolderPath(folderId, KF_FLAG_DEFAULT, nullptr, &rawPath)))
        {
            path = rawPath;
        }
        CoTaskMemFree(rawPath);
        return path;
    }

    auto add_known_folders(std::vector<directory_item>& roots) -> void
    {
        constexpr std::tuple<const KNOWNFOLDERID*, const char*, root_kind> s_knownFolders[] = {
            {&FOLDERID_Profile, "Home", root_kind::home},
            {&FOLDERID_Desktop, "Desktop", root_kind::desktop},
            {&FOLDERID_Documents, "Documents", root_kind::documents},
            {&FOLDERID_Downloads, "Downloads", root_kind::downloads},
            {&FOLDERID_Music, "Music", root_kind::music},
            {&FOLDERID_Videos, "Videos", root_kind::videos},
        };

        for (const auto& [folderId, name, kind] : s_knownFolders)
        {
            if (std::filesystem::path path = get_known_folder(*folderId); !path.empty())
            {
                directory_item& item = roots.emplace_back();
                item.path            = std::move(path);
                item.name            = name;
                item.rootKind        = kind;
                item.isDirectory     = true;
            }
        }
    }
}  // namespace

auto list_directory(const std::filesystem::path& directory) -> tl::expected<std::vector<directory_item>, std::string>
{
    std::wstring searchPattern = to_extended_length_path(directory);
    if (!searchPattern.ends_with(L'\\'))
    {
        searchPattern += L'\\';
    }
    searchPattern += L'*';

    WIN32_FIND_DATAW findData{};
    const HANDLE findHandle = FindFirstFileExW(searchPattern.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);

    if (findHandle == INVALID_HANDLE_VALUE)
    {
        const DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND)
        {
            return std::vector<directory_item>();
        }
        return tl::unexpected(win32_error_message(error));
    }

    std::vector<directory_item> items;

    do
    {
        const std::wstring_view name = findData.cFileName;
        if (name == L"." || name == L"..")
        {
            continue;
        }

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        {
            continue;
        }

        directory_item& item = items.emplace_back();
        item.path            = directory / std::filesystem::path(name);
        item.name            = path_to_utf8(std::filesystem::path(name));
        item.isDirectory     = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        item.sizeBytes       = (static_cast<std::uint64_t>(findData.nFileSizeHigh) << 32) | findData.nFileSizeLow;
        item.attributes      = findData.dwFileAttributes;
        item.reparseTag      = (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ? findData.dwReserved0 : 0;
    } while (FindNextFileW(findHandle, &findData));

    const DWORD finalError = GetLastError();
    FindClose(findHandle);

    if (finalError != ERROR_NO_MORE_FILES)
    {
        return tl::unexpected(win32_error_message(finalError));
    }

    return items;
}

auto list_roots() -> std::vector<directory_item>
{
    std::vector<directory_item> roots;
    add_known_folders(roots);

    std::wstring drives(GetLogicalDriveStringsW(0, nullptr), L'\0');
    GetLogicalDriveStringsW(static_cast<DWORD>(drives.size()), drives.data());

    for (const wchar_t* drive = drives.c_str(); *drive != L'\0'; drive += std::wcslen(drive) + 1)
    {
        directory_item& item = roots.emplace_back();
        item.path            = std::filesystem::path(drive);
        item.isDirectory     = true;

        // Volume labels are skipped: GetVolumeInformation can stall for seconds on a disconnected network drive.
        const std::string letter = path_to_utf8(item.path);
        switch (GetDriveTypeW(drive))
        {
            case DRIVE_REMOTE:
                item.name     = fmt::format("{}  (Network)", letter);
                item.rootKind = root_kind::network_drive;
                break;
            case DRIVE_REMOVABLE:
                item.name     = fmt::format("{}  (Removable)", letter);
                item.rootKind = root_kind::removable_drive;
                break;
            case DRIVE_CDROM:
                item.name     = fmt::format("{}  (Optical)", letter);
                item.rootKind = root_kind::optical_drive;
                break;
            default:
                item.name     = letter;
                item.rootKind = root_kind::local_drive;
                break;
        }
    }

    return roots;
}

auto describe_storage(const directory_item& item) -> std::string
{
    std::vector<std::string> parts;

    if (item.reparseTag != 0)
    {
        parts.push_back(fmt::format("Reparse point: {}", reparse_tag_name(item.reparseTag)));
    }

    constexpr std::pair<std::uint32_t, const char*> s_attributeNames[] = {
        {FILE_ATTRIBUTE_SPARSE_FILE, "Sparse"},
        {FILE_ATTRIBUTE_OFFLINE, "Offline"},
        {FILE_ATTRIBUTE_RECALL_ON_OPEN, "Recall on open"},
        {FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS, "Recall on data access"},
        {FILE_ATTRIBUTE_COMPRESSED, "Compressed"},
        {FILE_ATTRIBUTE_ENCRYPTED, "Encrypted"},
        {FILE_ATTRIBUTE_SYSTEM, "System"},
        {FILE_ATTRIBUTE_READONLY, "Read only"},
    };

    for (const auto& [attribute, name] : s_attributeNames)
    {
        if (item.attributes & attribute)
        {
            parts.emplace_back(name);
        }
    }

    if (parts.empty())
    {
        return "Regular file";
    }

    std::string description = parts.front();
    for (std::size_t index = 1; index < parts.size(); ++index)
    {
        description += ", " + parts[index];
    }
    return description;
}

#else

auto list_directory(const std::filesystem::path& directory) -> tl::expected<std::vector<directory_item>, std::string>
{
    std::error_code error;
    std::filesystem::directory_iterator iterator(directory, std::filesystem::directory_options::skip_permission_denied, error);

    if (error)
    {
        return tl::unexpected(error.message());
    }

    std::vector<directory_item> items;

    for (const std::filesystem::directory_entry& entry : iterator)
    {
        const std::string name = path_to_utf8(entry.path().filename());
        if (name.starts_with('.'))
        {
            continue;
        }

        directory_item& item = items.emplace_back();
        item.path            = entry.path();
        item.name            = name;
        item.isDirectory     = entry.is_directory(error);
        item.sizeBytes       = item.isDirectory ? 0 : entry.file_size(error);
    }

    return items;
}

auto list_roots() -> std::vector<directory_item>
{
    std::vector<directory_item> roots;

    const auto add_root = [&roots](std::filesystem::path path, std::string name, root_kind kind)
    {
        directory_item& item = roots.emplace_back();
        item.path            = std::move(path);
        item.name            = std::move(name);
        item.rootKind        = kind;
        item.isDirectory     = true;
    };

    if (const char* const home = std::getenv("HOME"); home && *home)
    {
        const std::filesystem::path homePath(home);
        add_root(homePath, "Home", root_kind::home);

        // macOS calls it Movies, Linux desktops call it Videos.
        constexpr std::pair<const char*, root_kind> s_homeFolders[] = {
            {"Desktop", root_kind::desktop},
            {"Documents", root_kind::documents},
            {"Downloads", root_kind::downloads},
            {"Music", root_kind::music},
            {"Movies", root_kind::videos},
            {"Videos", root_kind::videos},
        };

        for (const auto& [name, kind] : s_homeFolders)
        {
            std::error_code error;
            if (const std::filesystem::path folder = homePath / name; std::filesystem::is_directory(folder, error))
            {
                add_root(folder, name, kind);
            }
        }
    }

    add_root("/", "/", root_kind::local_drive);
    return roots;
}

auto describe_storage(const directory_item& item) -> std::string
{
    return "Regular file";
}

#endif
