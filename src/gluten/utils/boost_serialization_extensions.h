#pragma once

#include <filesystem>
#include "boost/serialization/nvp.hpp"

namespace boost::serialization
{
    static inline const char* pathEntryName = "path";

    template<class archive_class>
    auto save(archive_class& archive, const std::filesystem::path& path, const unsigned int fileVersion) -> void
    {
        std::wstring toSave = path.wstring();
        archive & boost::serialization::make_nvp(pathEntryName, toSave);
    }

    template<class archive_class>
    auto load(archive_class& archive, std::filesystem::path& path, const unsigned int fileVersion) -> void
    {
        std::wstring loaded;
        archive & boost::serialization::make_nvp(pathEntryName, loaded);
        path = loaded;
    }

    template<class archive_class>
    auto serialize(archive_class& archive, std::filesystem::path& path, const unsigned int fileVersion) -> void
    {
        boost::serialization::split_free(archive, path, fileVersion);
    }
}