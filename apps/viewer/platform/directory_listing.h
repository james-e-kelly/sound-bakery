#pragma once

#include "tl/expected.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

/**
 * @brief What a root view entry points at, so the UI can pick an icon for it.
 */
enum class root_kind : std::uint8_t
{
    none,  //< A regular entry inside a directory
    home,
    desktop,
    documents,
    downloads,
    music,
    videos,
    local_drive,
    network_drive,
    removable_drive,
    optical_drive,
};

/**
 * @brief One entry of a directory, as the OS reported it.
 */
struct directory_item
{
    std::filesystem::path path;
    std::string name;  //< UTF-8, for display
    root_kind rootKind       = root_kind::none;
    bool isDirectory         = false;
    std::uint64_t sizeBytes  = 0;
    std::uint32_t attributes = 0;  //< Win32 FILE_ATTRIBUTE_* flags
    std::uint32_t reparseTag = 0;  //< Win32 IO_REPARSE_TAG_* value, 0 when the entry isn't a reparse point
};

/**
 * @brief Lists a directory without following or filtering reparse points.
 *
 * On Windows this deliberately avoids std::filesystem's file type checks. Files on deduplicated volumes
 * (and other storage tiers like cloud placeholders) are reparse points, which those checks can report
 * as something other than a regular file. Anything without the directory attribute is treated as a file.
 * Hidden entries are skipped, matching Explorer's default.
 */
auto list_directory(const std::filesystem::path& directory) -> tl::expected<std::vector<directory_item>, std::string>;

/**
 * @brief The root locations to show above drive level: the user's common folders (Desktop, Documents...)
 * followed by drive letters on Windows, or "/" elsewhere.
 */
auto list_roots() -> std::vector<directory_item>;

/**
 * @brief Human readable summary of an item's attributes and reparse tag, for diagnosing storage issues.
 */
auto describe_storage(const directory_item& item) -> std::string;

auto path_to_utf8(const std::filesystem::path& path) -> std::string;
auto utf8_to_path(std::string_view utf8) -> std::filesystem::path;
