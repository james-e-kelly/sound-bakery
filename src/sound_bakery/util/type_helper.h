#pragma once

#include "sound_bakery/pch.h"

namespace SB::Util
{
    class TypeHelper final
    {
    public:
        static SB_OBJECT_CATEGORY getCategoryFromType(rttr::type type);

        static std::unordered_set<rttr::type> getTypesFromCategory(SB_OBJECT_CATEGORY category);

        static std::string_view getDisplayNameFromType(rttr::type type);

        static std::string getFolderNameForObjectType(rttr::type type);

        static std::string_view getFileExtensionOfObjectCategory(SB_OBJECT_CATEGORY category);

        static std::string_view getPayloadFromType(rttr::type type);
    };
}